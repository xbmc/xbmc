## Questions about Kodi?

To get your questions answered, please ask in the [Kodi community forums] (http://forum.kodi.tv/) or on IRC: #kodi on freenode.net

Do not open an issue.

## Issue Reports

Kodi uses github for development only, i.e. for pull requests and the discussion of code.

So we use a hook script to automatically close new issue created by you.

If you can, we encourage you to investigate the issue yourself and create a Pull Request for us to review.

For bug reports, feature requests and all other support, please go to [Kodi community forums] (http://forum.kodi.tv/).

## Pull Requests

- Before creating a pull request please read our general code guidelines that can be found at
  [Kodi development] (http://kodi.wiki/view/Development).

- **Create topic branches**. Don't ask us to pull from your master branch.

- **One pull request per feature**. If you want to do more than one thing, send
  multiple pull requests.

- **Send coherent history**. Make sure each individual commit in your pull
  request is meaningful. If you had to make multiple intermediate commits while
  developing, please squash them before sending them to us. In the end before merging you may be asked to squash your commit even some more.

Please follow this process; it's the best way to get your work included in the project:

- [Fork](http://help.github.com/fork-a-repo/) the project, clone your fork,
   and configure the remotes:

```bash
   # clone your fork of the repo into the current directory in terminal
   git clone git@github.com:<your username>/xbmc.git
   # navigate to the newly cloned directory
   cd xbmc
   # assign the original repo to a remote called "upstream"
   git remote add upstream https://github.com/xbmc/xbmc.git
   ```

- If you cloned a while ago, get the latest changes from upstream:

   ```bash
   # fetch upstream changes
   git fetch upstream
   # make sure you are on your 'master' branch
   git checkout master
   # merge upstream changes
   git merge upstream/master
   ```

- Create a new topic branch to contain your feature, change, or fix:

   ```bash
   git checkout -b <topic-branch-name>
   ```

- Commit your changes in logical chunks. or your pull request is unlikely
   be merged into the main project. Use git's
   [interactive rebase](https://help.github.com/articles/interactive-rebase)
   feature to tidy up your commits before making them public.

- Push your topic branch up to your fork:

   ```bash
   git push origin <topic-branch-name>
   ```

- [Open a Pull Request](https://help.github.com/articles/using-pull-requests) with a
    clear title and description.
