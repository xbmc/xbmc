#!/usr/bin/ruby

require 'fileutils.rb'
require 'find'
require 'rbconfig'
require 'optparse'

# Install directory - this contains a space to check
# for correct escaping of paths when passing comamnd
# line arguments under Windows
INSTALL_DIR = File.expand_path("install dir/")
PACKAGE_DIR = File.expand_path("package-dir/")
PACKAGE_SRC_DIR = File.expand_path("package-src-dir/")

if (RbConfig::CONFIG['host_os'] =~ /mswin|mingw/)
	OLDAPP_NAME = "oldapp.exe"
	NEWAPP_NAME = "newapp.exe"
	APP_NAME = "app.exe"
	UPDATER_NAME = "updater.exe"
	ZIP_TOOL = "C:/Cygwin/bin/zip.exe"
else
	OLDAPP_NAME = "oldapp"
	NEWAPP_NAME = "newapp"
	APP_NAME = "app"
	UPDATER_NAME = "updater"
	ZIP_TOOL = "zip"
end

file_list_vars = {
  "APP_FILENAME" => APP_NAME,
  "UPDATER_FILENAME" => UPDATER_NAME
}

def replace_vars(src_file,dest_file,vars)
	content = File.read(src_file)
	vars.each do |key,value|
		content.gsub! "$#{key}",value
	end
	File.open(dest_file,'w') do |file|
		file.print content
	end
end

def zip_supports_bzip2(zip_tool)
	# Try making an empty zip file with bzip2 compression, if bzip2 is not
	# supported, the tool will output an error, otherwise it will output
	# "Nothing to do"
	return `#{zip_tool} -Z bzip2 testing-bzip2-support.zip`.strip.include?("Nothing to do")
end

# Returns true if |src_file| and |dest_file| have the same contents, type
# and permissions or false otherwise
def compare_files(src_file, dest_file)
	if File.ftype(src_file) != File.ftype(dest_file)
		$stderr.puts "Type of file #{src_file} and #{dest_file} differ"
		return false
	end

	if File.file?(src_file) && !FileUtils.identical?(src_file, dest_file)
		$stderr.puts "Contents of file #{src_file} and #{dest_file} differ"
		return false
	end

	src_stat = File.stat(src_file)
	dest_stat = File.stat(dest_file)

	if src_stat.mode != dest_stat.mode
		$stderr.puts "Permissions of #{src_file} and #{dest_file} differ"
		return false
	end

	return true
end

# Compares the contents of two directories and returns a map of (file path => change type)
# for files and directories which differ between the two
def compare_dirs(src_dir, dest_dir)
	src_dir += '/' if !src_dir.end_with?('/')
	dest_dir += '/' if !dest_dir.end_with?('/')

	src_file_map = {}
	Find.find(src_dir) do |src_file|
		src_file = src_file[src_dir.length..-1]
		src_file_map[src_file] = nil
	end

	change_map = {}
	Find.find(dest_dir) do |dest_file|
		dest_file = dest_file[dest_dir.length..-1]

		if !src_file_map.include?(dest_file)
			change_map[dest_file] = :deleted
		elsif !compare_files("#{src_dir}/#{dest_file}", "#{dest_dir}/#{dest_file}")
			change_map[dest_file] = :updated
		end

		src_file_map.delete(dest_file)
	end

	src_file_map.each do |file|
		change_map[file] = :added
	end

	return change_map
end

force_elevation = false
run_in_debugger = false

OptionParser.new do |parser|
	parser.on("-f","--force-elevated","Force the updater to elevate itself") do
		force_elevation = true
	end
	parser.on("-d","--debug","Run the updater under GDB") do
		run_in_debugger = true
	end
end.parse!

BZIP2_AVAILABLE = zip_supports_bzip2(ZIP_TOOL)
if (BZIP2_AVAILABLE)
	ZIP_FLAGS = "-Z bzip2"
else
	ZIP_FLAGS = ""
end

if (BZIP2_AVAILABLE)
	puts "Using bzip2 compression"
else
	puts "Using plain old deflate compression - the 'zip' tool does not support bzip2"
end

# Remove the install and package dirs if they
# already exist
FileUtils.rm_rf(INSTALL_DIR)
FileUtils.rm_rf(PACKAGE_DIR)
FileUtils.rm_rf(PACKAGE_SRC_DIR)

# Create the install directory with the old app
Dir.mkdir(INSTALL_DIR)
FileUtils.cp(OLDAPP_NAME,"#{INSTALL_DIR}/#{APP_NAME}")

# Create a dummy file to uninstall
uninstall_test_file = "#{INSTALL_DIR}/file-to-uninstall.txt"
File.open(uninstall_test_file,"w") do |file|
	file.puts "this file should be removed after the update"
end

# Populate package source dir with files to install
Dir.mkdir(PACKAGE_SRC_DIR)
nested_dir_path = "#{PACKAGE_SRC_DIR}/new-dir/new-dir2"
FileUtils.mkdir_p(nested_dir_path)
nested_dir_test_file = "#{nested_dir_path}/new-file.txt"
File.open(nested_dir_test_file,'w') do |file|
	file.puts "this is a new file in a new nested dir"
end

FileUtils.cp(NEWAPP_NAME,"#{PACKAGE_SRC_DIR}/#{APP_NAME}")

# Create .zip packages from source files
Dir.mkdir(PACKAGE_DIR)
Dir.chdir(PACKAGE_SRC_DIR) do
	if !system("#{ZIP_TOOL} #{ZIP_FLAGS} -r #{PACKAGE_DIR}/app-pkg.zip .")
		raise "Unable to create update package"
	end
end

# Copy the install script and updater to the target
# directory
replace_vars("file_list.xml","#{PACKAGE_DIR}/file_list.xml",file_list_vars)
FileUtils.cp("../#{UPDATER_NAME}","#{PACKAGE_DIR}/#{UPDATER_NAME}")

# Run the updater using the new syntax
#
# Run the application from the install directory to
# make sure that it looks in the correct directory for
# the file_list.xml file and packages
#
install_path = File.expand_path(INSTALL_DIR)
Dir.chdir(INSTALL_DIR) do
	flags = "--force-elevated" if force_elevation
	debug_flags = "gdb --args" if run_in_debugger
	cmd = "#{debug_flags} #{PACKAGE_DIR}/#{UPDATER_NAME} #{flags} --install-dir \"#{install_path}\" --package-dir \"#{PACKAGE_DIR}\" --script file_list.xml"
	puts "Running '#{cmd}'"
	system(cmd)
end

# TODO - Correctly wait until updater has finished
sleep(1)

# Check that the app was updated
app_path = "#{INSTALL_DIR}/#{APP_NAME}"
output = `"#{app_path}"`
if (output.strip != "new app starting")
	throw "Updated app produced unexpected output: #{output}"
end

# Check that the packaged dir and install dir match
dir_diff = compare_dirs(PACKAGE_SRC_DIR, INSTALL_DIR)
ignored_files = ["test-dir", "test-dir/app-symlink", UPDATER_NAME]
have_unexpected_change = false
dir_diff.each do |path, change_type|
	if !ignored_files.include?(path)
		case change_type
		when :added
			$stderr.puts "File #{path} was not installed"
		when :changed
			$stderr.puts "File #{path} differs between install and package dir"
		when :deleted
			$stderr.puts "File #{path} was not uninstalled"
		end
		have_unexpected_change = true
	end
end

if have_unexpected_change
	throw "Unexpected differences between packaging and update dir"
end

puts "Test passed"
