This tool is a component of a cross-platform auto-update system.
It is responsible for performing the installation of an update after
the necessary files have been downloaded to a temporary directory.

It was originally written for use with Mendeley Desktop (see www.mendeley.com)

The tool consists of a single small binary which performs update installation,
an XML file format describing the contents of an update (an 'update script') 
and a tool to create update scripts from a directory containing an installed application.

To perform an update, the application (or another separate tool) needs to download
the updater binary, an update script and one or more compressed packages
containing the files for the update to a temporary directory.  It then needs
to invoke the updater, specifying the location where the application is installed,
the location of the compressed packages and the path to the update script.

Once the updater has been started, it:

 1. Waits for the application to exit
 2. Acquires the necessary priviledges to install the updates, prompting
    the user if necessary.
 3. Installs the updates, displaying progress to the user in a small dialog 
 4. Performs cleanup and any additional actions required as part of the update
 5. Starts the new version of the main application.

 In the event of a failure during the update, the installation is rolled back
 to its previous state and a message is presented to the user.

## Building the Updater

 Create a new directory for the build and from that directory run:

    cmake <path to source directory>
    make

 The updater binary will be built in the src/ directory.

 You should also run the tests in src/tests to verify that the updater is
 functioning correctly.

## Preparing an Update

 1. Create a directory containing your application's files,
    laid out in the same way and with the same permissions as they would be when installed.
 2. Create a config file specifying how the application's files should be
    partitioned into packages - see tools/config-template.json
 3. Use the tools/create-packages.rb script to create a file_list.xml file
    and a set of package files required for updates.
 4. Upload the file_list.xml file and packages to a server

 After step 4 is done, you need to notify existing installs that an update
 is available.  The installed application then needs to download the
 relevant packages, file_list.xml file and updater binary to a temporary
 directory and invoke the updater.

 See doc/update-hosting for more details on hosting and delivering the updates.

## Invoking the Updater

 Once the application has downloaded an update, it needs to invoke it.  The syntax is:

    updater --install-dir <install-dir> --package-dir <package-dir> --script <script file>

 Where `<install-dir>` is the directory which the application is installed into,
 `<package-dir>` is the directory containing the packages required for the update
 and `<script>` is the `file_list.xml` file describing the update.

 Once the updater has run, it will launch the file specified in the `file_list.xml` file
 as being the main application binary.

 See the updater test in `src/tests/test-update.rb` for an example
 of how to invoke the updater.

 You should design the process used to download and launch the updater so that new
 versions of the updater itself can be delivered as part of the update if necessary.

## Customizing the Updater

 To customize the application name, organization and messages displayed by the updater:

  1. Edit the AppInfo class (in AppInfo.h, AppInfo.cpp) to set the name
     of the application and associated organization.
  2. Replace the icons in src/resources
  3. Change the product name and organization in src/resources/updater.rc
  4. If you are building the updater on Windows and have a suitable Authenticode
     certificate, use it to sign the Windows binary.  This will make the application
	 show a less scary UAC prompt if administrator permissions are required
	 to complete the installation.

## Updater Dependencies

 The external dependencies of the updater binary are:

 * The C/C++ runtime libraries (Linux, Mac),
 * pthreads (Linux, Mac),
 * zlib (Linux, Mac)
 * native UI library (Win32 API on Windows, Cocoa on Mac, GTK on Linux if available)

## Full and Delta Updates

 The simplest auto-update implementation is for existing installs
 to download a complete copy of the new version and install it.  This is
 appropriate if a full download and install will not take a long time for most users
 (eg. if the application is small or they have a fast internet connection).

 With this tool, a full-update involves putting all files in a build of
 the application into a single package.

 To reduce the download size, delta updates can be created which only include
 the necessary files or components to update from the old to the new version.

 The file_list.xml file format can be used to represent either a complete
 install - in which every file that makes up the application is included,
 or a delta update - in which case only new or updated files and packages
 are included.

 There are several ways in which this can be done:

 * Pre-computed Delta Updates
  For each release, create a full update plus delta updates from the
  previous N releases.  Users of recent releases will receive a small
  delta update.  Users of older releases will receive the full update.

 * Server-computed Delta Updates
  The server receives a request for an update from client version X and in response,
  computes an update from version X to the current version Y, possibly
  caching that information for future use.  The client then receives the
  delta file_list.xml file and downloads only the listed packages.

  Applications such as Chrome and Firefox use a mixture of the above methods.

 * Client-computed Delta Updates
  The client downloads the file_list.xml file for the latest version and
  computes a delta update file locally.  It then downloads only the required
  packages and invokes the updater, which installs only the changed or updated
  files from those packages.

  This is similar to Linux package management systems.
