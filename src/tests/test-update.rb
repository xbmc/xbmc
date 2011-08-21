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
output = `#{INSTALL_DIR}/app`
if (output.strip == "new app starting")
	puts "Updated app produced expected output"
	exit(0)
else
	puts "Updated app produced unexpected output: #{output}"
	exit(1)
end

