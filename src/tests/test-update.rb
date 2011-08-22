#!/usr/bin/ruby

require 'fileutils.rb'

INSTALL_DIR = "install-dir/"
PACKAGE_DIR = "package-dir/"

# Remove the install and package dirs if they
# already exist
FileUtils.rm_rf(INSTALL_DIR)
FileUtils.rm_rf(PACKAGE_DIR)

# Create the install directory with the old app
Dir.mkdir(INSTALL_DIR)
FileUtils.cp("oldapp","#{INSTALL_DIR}/app")

# Create a dummy file to uninstall
uninstall_test_file = "#{INSTALL_DIR}/file-to-uninstall.txt"
File.open(uninstall_test_file,"w") do |file|
	file.puts "this file should be removed after the update"
end

# Create the update archive containing the new app
Dir.mkdir(PACKAGE_DIR)
FileUtils.cp("newapp","#{PACKAGE_DIR}/app")
system("zip #{PACKAGE_DIR}/app-pkg.zip -j #{PACKAGE_DIR}/app")
FileUtils.rm("#{PACKAGE_DIR}/app")

# Copy the install script and updater to the target
# directory
FileUtils.cp("file_list.xml","#{PACKAGE_DIR}/file_list.xml")
FileUtils.cp("../updater","#{PACKAGE_DIR}/updater")

# Run the updater using the new syntax
system("#{PACKAGE_DIR}/updater --install-dir #{INSTALL_DIR} --package-dir #{PACKAGE_DIR} --script #{PACKAGE_DIR}/file_list.xml")

# TODO - Correctly wait until updater has finished
sleep(1)

# Check that the app was updated
app_path = "#{INSTALL_DIR}/app"
output = `#{app_path}`
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

