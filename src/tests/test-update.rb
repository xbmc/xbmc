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
IS_WINDOWS = RbConfig::CONFIG['host_os'] =~ /mswin|mingw/

if IS_WINDOWS
	OLDAPP_NAME = "oldapp.exe"
	NEWAPP_NAME = "newapp.exe"
	APP_NAME = "app.exe"
	UPDATER_NAME = "updater.exe"
	ZIP_TOOL = File.expand_path("../zip-tool.exe")
else
	OLDAPP_NAME = "oldapp"
	NEWAPP_NAME = "newapp"
	APP_NAME = "app"
	UPDATER_NAME = "updater"
	ZIP_TOOL = File.expand_path("../zip-tool")
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

def create_test_file(name, content)
	File.open(name, 'w') do |file|
		file.puts content
	end
	return name
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

# copy 'src' to 'dest', preserving the attributes
# of 'src'
def copy_file(src, dest)
	FileUtils.cp src, dest, :preserve => true
end

# Remove the install and package dirs if they
# already exist
FileUtils.rm_rf(INSTALL_DIR)
FileUtils.rm_rf(PACKAGE_DIR)
FileUtils.rm_rf(PACKAGE_SRC_DIR)

# Create the install directory with the old app
Dir.mkdir(INSTALL_DIR)
copy_file OLDAPP_NAME, "#{INSTALL_DIR}/#{APP_NAME}"

# Create a dummy file to uninstall
uninstall_test_file = create_test_file("#{INSTALL_DIR}/file-to-uninstall.txt", "this file should be removed after the update")
uninstall_test_symlink = if not IS_WINDOWS
	FileUtils.ln_s("#{INSTALL_DIR}/file-to-uninstall.txt", "#{INSTALL_DIR}/symlink-to-file-to-uninstall.txt")
else
	create_test_file("#{INSTALL_DIR}/symlink-to-file-to-uninstall.txt", "dummy file.  this is a symlink on Unix")
end

# Populate package source dir with files to install
Dir.mkdir(PACKAGE_SRC_DIR)
nested_dir_path = "#{PACKAGE_SRC_DIR}/new-dir/new-dir2"
FileUtils.mkdir_p(nested_dir_path)
FileUtils::chmod 0755, "#{PACKAGE_SRC_DIR}/new-dir"
FileUtils::chmod 0755, "#{PACKAGE_SRC_DIR}/new-dir/new-dir2"
nested_dir_test_file = "#{nested_dir_path}/new-file.txt"
File.open(nested_dir_test_file,'w') do |file|
	file.puts "this is a new file in a new nested dir"
end
FileUtils::chmod 0644, nested_dir_test_file
copy_file NEWAPP_NAME, "#{PACKAGE_SRC_DIR}/#{APP_NAME}"
FileUtils::chmod 0755, "#{PACKAGE_SRC_DIR}/#{APP_NAME}"

# Create .zip packages from source files
Dir.mkdir(PACKAGE_DIR)
Dir.chdir(PACKAGE_SRC_DIR) do
	if !system("#{ZIP_TOOL} #{PACKAGE_DIR}/app-pkg.zip .")
		raise "Unable to create update package"
	end
end

# Copy the install script and updater to the target
# directory
replace_vars("file_list.xml","#{PACKAGE_DIR}/file_list.xml",file_list_vars)
copy_file "../#{UPDATER_NAME}", "#{PACKAGE_DIR}/#{UPDATER_NAME}"

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
	cmd = "#{debug_flags} #{PACKAGE_DIR}/#{UPDATER_NAME} #{flags} --install-dir \"#{install_path}\" --package-dir \"#{PACKAGE_DIR}\" --script file_list.xml --auto-close"
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
