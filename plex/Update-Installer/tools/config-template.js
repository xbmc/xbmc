{
	"packages" : {
		// a single package which contains all files for the application
		"app" : [
			".*"
		],
	},
	
	// the name of the updater binary - this will be listed as
	// a dependency of the update process
	"updater-binary" : "updater",

	// the name of the main binary to launch when the
	// application starts
	"main-binary" : "myapp"
}
