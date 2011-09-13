#!/usr/bin/ruby

require 'fileutils'
require 'rubygems'
require 'find'
require 'json'
require 'rexml/document'
require 'optparse'

# syntax:
#
# create-packages.rb <input directory> <config file> <output directory>
#
#  Takes the set of files that make up a release and splits them up into
#  a set of .zip packages along with a file_list.xml file listing all
#  the files in the release and mapping them to their respective
#  packages.
#
#  Outputs:
#
#   <output-dir>/$PACKAGE_NAME.zip
#     These are packages containing the files in this version of the software.
#
#   <output-dir>/file_list.xml
#     This file lists all the files contained in this version of the software and
#     the packages which they are contained in.
#
#   <output-dir>/$UPDATER_BINARY
#     The standalone auto-update installer binary.  This is not compressed so that
#     it can be downloaded and executed directly.
#

# Represents a group of updates in a release
class UpdateScriptPackage
	# name - The name of the package (without any extension)
	# hash - The SHA-1 hash of the package
	# size - The size of the package in bytes
	attr_reader :name,:hash,:size
	attr_writer :name,:hash,:size
end

# Represents a single file in a release
class UpdateScriptFile
	# path - The path of the file relative to the installation directory
	# hash - The SHA-1 hash of the file
	# permissions - The permissions of the file (See File::stat)
	# size - The size of the file in bytes
	# package - The name of the package containing this file
	attr_reader :path,:hash,:permissions,:size,:package,:target,:is_main_binary
	attr_writer :path,:hash,:permissions,:size,:package,:target,:is_main_binary
end

# Utility method - convert a hash map to an REXML element
#
# Hash keys are converted to elements and hash values either
# to text contents or child elements (if the value is a Hash)
#
# 'root' - the root REXML::Element
# 'map' - a hash mapping element names to text contents or
#         hash maps
#
def hash_to_xml(root,map)
	map.each do |key,value|
		element = REXML::Element.new(key)
		if value.instance_of?(String)
			element.text = value
		elsif value.instance_of?(Hash)
			hash_to_xml(element,value)
		elsif !value.nil?
			raise "Unsupported value type #{value.class}"
		end
		root.add_element element
	end
	return root
end

def strip_prefix(string,prefix)
	if (!string.start_with?(prefix))
		raise "String does not start with prefix"
	end
	return string.sub(prefix,"")
end

def file_sha1(path)
		return `sha1sum "#{path}"`.split(' ')[0]
end

class UpdateScriptGenerator

	# target_version - The version string for the build in this update
	# platform - The platform which this build is for
	# input_dir - The directory containing files that make up the install
	# output_dir - The directory containing the generated packages
	# package_config - The PackageConfig specifying the file -> package map
	#                  for the application and other config options
	# file_list - A list of all files in 'input_dir' which make up the install
	# package_file_map - A map of (package name -> [paths of files in this package])

	def initialize(target_version,
	               platform,
	               input_dir,
	               output_dir,
	               package_config,
	               file_list,
	               package_file_map)

		@target_version = target_version
		@platform = platform
		@config = package_config

		# List of files to install in this version
		@files_to_install = []
		file_list.each do |path|
			file = UpdateScriptFile.new
			file.path = strip_prefix(path,input_dir)

			if (File.basename(path) == @config.updater_binary)
				# for the updater binary, use the possibly substituted
				# version in the output directory
				path = "#{output_dir}/#{@config.updater_binary}"	
			end

			file.is_main_binary = (file.path == package_config.main_binary)
			
			if (File.symlink?(path))
				file.target = File.readlink(path)
			else
				file.hash = file_sha1(path)
				file.permissions = File.stat(path).mode
				file.size = File.size(path)
				file.package = @config.package_for_file(file.path)

				if (!file.package)
					raise "Could not find package for file #{file.path}"
				end
			end

			@files_to_install << file
		end

		# List of packages containing files for this version
		@packages = []
		package_file_map.each do |package_name,files|
			if (package_name == @config.updater_binary)
				path = "#{output_dir}/#{package_name}"
			else
				path = "#{output_dir}/#{package_name}.zip"
			end

			package = UpdateScriptPackage.new
			package.name = package_name
			package.size = File.size(path)
			package.hash = file_sha1(path)
			@packages << package
		end
	end

	def to_xml()
		doc = REXML::Document.new
		update_elem = REXML::Element.new("update")
		doc.add_element update_elem

		version_elem = REXML::Element.new("targetVersion")
		version_elem.text = @target_version

		platform_elem = REXML::Element.new("platform")
		platform_elem.text = @platform

		update_elem.add_attribute("version","3")
		update_elem.add_element version_elem
		update_elem.add_element platform_elem
		update_elem.add_element deps_to_xml()
		update_elem.add_element packages_to_xml()
		update_elem.add_element install_to_xml()

		output = ""
		doc.write output
		return output
	end

	def deps_to_xml()
		deps_elem = REXML::Element.new("dependencies")
		deps = @config.updater_binary
		deps.each do |dependency|
			dep_elem = REXML::Element.new("file")
			dep_elem.text = dependency
			deps_elem.add_element dep_elem
		end
		return deps_elem
	end

	def packages_to_xml()
		packages_elem = REXML::Element.new("packages")
		@packages.each do |package|
			package_elem = REXML::Element.new("package")
			packages_elem.add_element package_elem
			hash_to_xml(package_elem,{
				"name" => package.name,
				"size" => package.size.to_s,
				"hash" => package.hash
			})
		end
		return packages_elem
	end

	def install_to_xml()
		install_elem = REXML::Element.new("install")
		@files_to_install.each do |file|
			file_elem = REXML::Element.new("file")
			install_elem.add_element(file_elem)

			attributes = {"name" => file.path}
			if (file.target)
				attributes["target"] = file.target
			else
				attributes["size"] = file.size.to_s
				attributes["permissions"] = file_mode_string(file.permissions)
				attributes["hash"] = file.hash
				attributes["package"] = file.package
			end

			if (file.is_main_binary)
				attributes["is-main-binary"] = "true"
			end

			hash_to_xml(file_elem,attributes)
		end
		return install_elem
	end

	def file_mode_string(mode)
		mode_octal = (mode & 0777).to_s(8)
		return "0#{mode_octal}"
	end
end

class PackageConfig
	attr_reader :main_binary, :updater_binary

	def initialize(map_file)
		@rule_map = {}
		config_json = JSON.parse(File.read(map_file))
		config_json["packages"].each do |package,rules|
			rules.each do |rule|
				rule_regex = Regexp.new("^#{rule}")
				@rule_map[rule_regex] = package
			end
		end

		@main_binary = config_json["main-binary"]
		@updater_binary = config_json["updater-binary"]
	end

	def is_updater(file)
		return File.basename(file) == @updater_binary
	end

	def package_for_file(file)
		match = nil
		@rule_map.each do |rule,package|
			if (file =~ rule)
				if match && match != package
					raise "Multiple packages match file #{file} - found '#{match}' and '#{package}'"
				end
				match = package
			end
		end
		return match
	end
end

updater_binary_input_path = nil
target_version = nil
target_platform = nil
zip_flags = nil

OptionParser.new do |parser|
	parser.banner = "#{$0} [options] <input dir> <config file> <output dir>"
	parser.on("-u","--updater [updater binary]","Specifies the updater binary to use") do |updater|
		updater_binary_input_path = updater
	end
	parser.on("-v","--version [version]","Specifies the target version string for this update") do |version|
		target_version = version
	end
	parser.on("-p","--platform [platform]","Specifies the target platform for this update") do |platform|
		target_platform = platform
	end
	parser.on(nil,"--bzip2","Use bzip2 compression (requires that 'zip' supports the -Z bz2 argument)") do
		zip_flags = "-Z bzip2"
	end
end.parse!

raise "Platform not specified (use -p option)" if !target_platform
raise "Target version not specified (use -v option)" if !target_version

if ARGV.length < 3
	raise "Missing arguments"
end

input_dir = ARGV[0]
package_map_file = ARGV[1]
output_dir = ARGV[2]

FileUtils.mkpath(output_dir)

# get the details of each input file
input_file_list = []
Find.find(input_dir) do |path|
	next if (File.directory?(path) && !File.symlink?(path))
	input_file_list << path
end

# map each input file to a corresponding package

# read the package map
package_config = PackageConfig.new(package_map_file)

# map of package name -> array of files
package_file_map = {}

input_file_list.each do |file|
	next if File.symlink?(file)

	relative_file = strip_prefix(file,input_dir)
	package = package_config.package_for_file(relative_file)
	if (!package)
		raise "Unable to find package for file #{file}"
	end
	package_file_map[package] = [] if !package_file_map[package]
	package_file_map[package] << file
end

# generate each package
package_file_map.each do |package,files|
	puts "Generating package #{package}"

	quoted_files = []
	files.each do |file|
		# do not package the updater binary into a zip file -
		# it must be downloaded uncompressed
		if package_config.is_updater(file)
			if (!updater_binary_input_path)
				updater_binary_input_path = file
			end
			next
		end
		quoted_files << "\"#{strip_prefix(file,input_dir)}\""
	end

	if (quoted_files.empty?)
		puts "Skipping generation of empty package #{package}"
		next
	end

	# sort the files in alphabetical order to ensure
	# that if the input files for a package have the same
	# name and content, the resulting package will have the
	# same SHA-1
	#
	# This means that repeated runs of the package creation tool
	# on the same set of files should generated packages with the
	# same SHA-1
	quoted_files.sort! do |a,b|
		a <=> b
	end
	
	quoted_file_list = quoted_files.join(" ")

	output_path = File.expand_path(output_dir)
	output_file = "#{output_path}/#{package}.zip"

	File.unlink(output_file) if File.exist?(output_file)

	Dir.chdir(input_dir) do
		if (!system("zip #{zip_flags} #{output_file} #{quoted_file_list}"))
			raise "Failed to generate package #{package}"
		else
			puts "Generated package #{package} : #{file_sha1(output_file)}"
		end
	end
end

# copy the updater to the output directory
puts "Using updater binary: #{updater_binary_input_path}"
if !updater_binary_input_path
	puts "Updater binary not found in input directory: #{input_dir}"
	exit(1)
end

FileUtils.cp(updater_binary_input_path,"#{output_dir}/#{File.basename(updater_binary_input_path)}")

# output the file_list.xml file
update_script = UpdateScriptGenerator.new(target_version,target_platform,input_dir,
                  output_dir,package_config,input_file_list,package_file_map)
output_xml_file = "#{output_dir}/file_list.unformatted.xml"
File.open(output_xml_file,'w') do |file|
	file.write update_script.to_xml()
end

# xmllint generates more readable formatted XML than REXML, so write unformatted
# XML first and then format it with xmllint.
system("xmllint --format #{output_xml_file} > #{output_dir}/file_list.xml")
File.delete(output_xml_file)


