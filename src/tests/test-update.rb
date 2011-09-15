#!/usr/bin/ruby

require 'fileutils.rb'
require 'rbconfig'
require 'optparse'

# Install directory - this contains a space to check
# for correct escaping of paths when passing comamnd
# line arguments under Windows
INSTALL_DIR = File.expand_path("install dir/")
PACKAGE_DIR = File.expand_path("package-dir/")

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

# Create the install directory with the old app
Dir.mkdir(INSTALL_DIR)
FileUtils.cp(OLDAPP_NAME,"#{INSTALL_DIR}/#{APP_NAME}")

# Create a dummy file to uninstall
uninstall_test_file = "#{INSTALL_DIR}/file-to-uninstall.txt"
File.open(uninstall_test_file,"w") do |file|
	file.puts "this file should be removed after the update"
end

# Create the update archive containing the new app
Dir.mkdir(PACKAGE_DIR)
FileUtils.cp(NEWAPP_NAME,"#{PACKAGE_DIR}/#{APP_NAME}")
system("#{ZIP_TOOL} #{ZIP_FLAGS} #{PACKAGE_DIR}/app-pkg.zip -j #{PACKAGE_DIR}/#{APP_NAME}")
FileUtils.rm("#{PACKAGE_DIR}/#{APP_NAME}")

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

# Check that the permissions were correctly set on the installed app
mode = File.stat(app_path).mode.to_s(8)
if (mode != "100755")
	throw "Updated app has incorrect permissions: #{mode}"
end

if (File.exist?(uninstall_test_file))
	throw "File to uninstall was not removed"
end

