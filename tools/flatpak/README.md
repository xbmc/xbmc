# Kodi Flatpak

Kodi is an award-winning free and open source software media player and
entertainment hub for digital media. Available as a native application for
Android, Linux, BSD, macOS, iOS, tvOS and Windows operating systems, Kodi runs
on most common processor architectures. This repository packages it through
Flatpak.

## Building

Then build via

```
flatpak-builder build-dir --user --ccache --force-clean --install tv.kodi.Kodi.yml
```

Then you can run it via the command line:

```
flatpak run tv.kodi.Kodi
```

or just search for the installed app on your system

The following binary addons do not compile, and are excluded:

- `audiodecoder.dumb`
- `game.libretro.2048`

## Contributing

The list of binary addons in each branch of Kodi may be found
[here](https://github.com/xbmc/repo-binary-addons/), and dependencies
[here](https://github.com/xbmc/xbmc/tree/master/tools/depends/target). Kodi
releases are found [here](https://github.com/xbmc/xbmc/releases).

You need to have the `PyGithub` Python module installed, to run the update script:

```sh
pip install PyGithub
```

`make update-addons` can help updating existing addons and also list missing ones. It will need a `GITHUB_TOKEN` environment variable set to a valid GitHub token. You can do this via an `.env` file in the root of the repository or by exporting the variable in your shell.

You can contribute by updating addons, modules and the Kodi version.
