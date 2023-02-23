![Kodi Logo](resources/banner_slim.png)

# Contributing to Kodi Home Theater
Looking to **contribute code** to Kodi Home Theater? Great! You landed on the right place and we appreciate you for taking the time. Can't code for your life? No shame in that. There's a lot more to the Kodi project than just code. Check out **[other ways](../README.md#how-to-contribute)** to contribute.

First, a few words of caution: Kodi's huge codebase spans over 15 years of development, hundreds of volunteer developers from all walks of life and countless, sleepless hours. Please remember that somewhere in between your first *WTH?* and before you start ranting or splitting hairs.

Also, always keep in mind that for many of us English is not our native language. What might come across as discourteous is probably an unintended side effect of the language barrier. Usage of emoticons is highly encouraged. They are great to convey, er, emotions and state of mind.

## Getting Started
Before you start coding we advise you to build Kodi's master branch. This will save you and us precious time by making sure a proper build environment is set up and all dependencies are met. For your convenience, **[build guides](README.md)** are available for most common platforms. A **[git guide](GIT-FU.md)** is also available, streamlined to Kodi's development workflow. Unless you're a *git ninja*, please read it carefully.  
If you are a *git ninja*, we feel happy and sad for you at the same time. Your soul might be lost to the dark side already but worry not, we have cookies!

## Reviews
Reviews are a great way to get familiar with the codebase. If you would like to start with reviewing just use **[this handy github filter](https://github.com/xbmc/xbmc/pulls?q=is%3Apr+is%3Aopen+review%3Anone)** to see PRs that are in need of one (we have plenty!).

## Pull request submission guidelines
* **Create topic branches**. Never, ever open a pull request from your master branch. **Ever!**
* **Code against master branch**. Unless release-specific, all Kodi development occurs in the `master` branch. If it needs backporting it can be done after it hits master.
* **One topic branch per change**. Found something that needs fixing but is unrelated to the current work? Create a new topic branch and open another *pull request*.
* **Use a coherent commit history**. Make sure each individual commit in your *pull request* is meaningful and organized in logical chunks. Tidy up and squash commits before submitting.
* **Be descriptive, but concise**. The *pull request* and *commits* should have a useful title and follow the format:<br/>
`[Component(s)] Short description...` or alternative format `Component(s): Short description...`<br/>
By "component", we mean that you should identify the area of the code (gui, video, depends, builds, etc), the code context (e.g. class name), etc. You can easily find hints by browsing through the git history. An example:<br/>
`[Python][xbmcgui] Fix x problem...` or `Python/xbmcgui: Fix x problem...`<br/>
instead of something like `[README.md] Update`, which is **not** descriptive enough.<br/>
Also, adding a short description to the *commit* (under the title) is a good thing to consider to give a better explanation of the commit changes.<br/>
The *pull request* description should only contain information relevant to the change.
* **Draft PR**. If your work requires further development, and you wish to ask the opinion of other developers on how to proceed, you can publish your PR as Draft.

## Pull request merge guidelines
* **First rule, be patient, be kind, rewind**. We're all volunteers with busy lives but usually very responsive in cataloging *pull requests* and pinging the relevant maintainer(s). Although not valid for the whole codebase, specific components of Kodi have a set of [maintainers/code owners](https://github.com/xbmc/xbmc/blob/master/.github/CODEOWNERS) whose review will be automatically requested when you open a pull request.
* **Jenkins code formatting requests**. Always apply code style formatting requests from Jenkins. If not, please provide a valid reason why you do not apply them.
* **Jenkins CI builds**. It is mandatory to have green status on all Jenkins building platforms. The "No-Jenkins" label can be set by maintainers to avoid having the CI build the PR code (and waste build resources) when the contribution is still a work-in-progress or if it's known in advance that it won't build.
* **Reviewing time**. Be patient. In general terms we're talking weeks, not days, but the active team is small and things can stack up, even with everyone doing their best. If your PR has been standing for too long without a review, feel free to let us know.
* **Reviewing process**. At least one member's approval is required before getting the green light to merge. Please take into account that expertise is spread thinly across areas of Kodi. This makes getting reviews of some changes even harder. Kodi is tangled and what looks perfect to you may have unwanted side effects, so approval by several members is sometimes required. In any case, this is always helpful, in particular for huge additions/refactors that may cause regressions. Therefore, it's always possible that your code will require more love (i.e. changes) before being ready to merge, so arm yourself with relaxing herbal teas.
* **Test builds**. When reasonable, providing test builds to test your changes can speed up the review process, especially if you consider that some users or team members cannot build Kodi for testing. If you are a member of Team Kodi, we remind you that there are Jenkins helpers that can build and upload code onto the mirrors where users can access it for test purposes. If you are not part of the team, you can make a request for test builds.

## Coding guidelines
* **Follow our [code guidelines](CODE_GUIDELINES.md)**. New code should follow those guidelines even if existing code doesn't. If you're up to it, improve existing code and submit it in separate commits.
* **Document the code**. When changes are made to the code, its functionality may be clear to the author, but other developers who read the block of code (even after several years) can easily have a hard time understanding it. Providing documentation saves time for everyone.
* **Separate code from cosmetics**. Don't mix code and unrelated cosmetic changes in the same commit. It's already hard to review code without added noise. Of course, deleting a **few** trailing spaces does not warrant a separate commit. Use your judgement. Also, note that the PR build job expects each individual commit to be formatted according to the [`.clang-format`](https://github.com/xbmc/xbmc/blob/master/.clang-format) rules, so if your change touches existing code that needs reformatting to satisfy the current rules, then those would be considered related cosmetic changes that should be included in the same commit.
* **Test your changes**. Kodi's **[continuous integration system](http://jenkins.kodi.tv/)** builds every PR automatically. Nonetheless, you're expected to test the code on your development platform.

## Updating your PR
Making a *pull request* adhere to the standards above can be difficult. If the maintainers notice anything that they'd like changed, they'll ask you to edit your *pull request* before it gets merged. **There's no need to open a new *pull request*, just edit the existing one**. If you're not sure how to do that, our **[git guide](GIT-FU.md)** provides a step by step guide. If you're still not sure, ask us for guidance. We're all fairly proficient with *git* and happy to be of assistance.

<a href="https://github.com/xbmc/xbmc"><img src="https://forthebadge.com/images/badges/made-with-c-plus-plus.svg" height="25"></a>
<a href="https://github.com/xbmc/xbmc"><img src="https://forthebadge.com/images/badges/contains-technical-debt.svg" height="25"></a>

