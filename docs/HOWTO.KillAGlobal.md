![Kodi Logo](resources/banner_slim.png)

# HOW-TO: Kill a Global
Guide originally published at **[PR13658](https://github.com/xbmc/xbmc/pull/13658)**. To better understand this document, you should read the original *Pull Request*.

## Table of Contents
1. **[What is the root issue?](#1-what-is-the-root-issue)**
2. **[Globals? Singletons? What's the difference?](#2-globals-singletons-whats-the-difference)**
3. **[What is the solution?](#3-what-is-the-solution)**
4. **[Step 1: Identify and remove the instance](#4-step-1-identify-and-remove-the-instance)**
5. **[Step 2: Add instance to CServiceManager](#5-step-2-add-instance-to-cservicemanager)**
6. **[Step 3: Move initialization and deinitialization to CServiceManager](#6-step-3-move-initialization-and-deinitialization-to-cservicemanager)**  
7. **[Step 4: Replace local dependencies with references](#7-step-4-replace-local-dependencies-with-references)**  
  7.1. **[Step 4.1 - Settings](#71-step-41---settings)**  
  7.2. **[Step 4.2 - Data Cache Core](#72-step-42---data-cache-core)**  
8. **[Step 5: Make service registration local](#8-step-5-make-service-registration-local)**
9. **[Step 6: Const-correctness and static cleanup](#9-step-6-const-correctness-and-static-cleanup)**
10. **[Step 7: Replace external uses with CServiceBroker calls](#10-step-7-replace-external-uses-with-cservicebroker-calls)**
11. **[Step 8: Cleanup](#11-step-8-cleanup)**
12. **[Step 9: Submit your PR](#12-step-9-submit-your-pr)**
13. **[Appendix: Dealing with the human aspect](#13-appendix-dealing-with-the-human-aspect)**

## 1. What is the root issue?
The root issue is that Kodi has a complicated dependency graph, so the order in which things are initialized/deinitialized is very important. When a class is global, we can't control this order, which is why things break.

**[back to top](#table-of-contents)**

## 2. Globals? Singletons? What's the difference?
A global is a variable not owned by a class, e.g.

```c++
CApplication g_application;
```

A singleton is a static variable owned by a class, e.g.

```c++
CStereoscopicsManager& CStereoscopicsManager::GetInstance()
{
  static CStereoscopicsManager sStereoscopicsManager;
  return sStereoscopicsManager;
}
```

With singletons, the constructor is called on first access, so as long as things are accessed in order, Kodi doesn't crash on startup.

However, on shutdown, singletons are effectively globals, because we don't control the order of destruction. Therefore, I sometimes refer to singletons as globals.

**[back to top](#table-of-contents)**

## 3. What is the solution?

Fernet has implemented a useful architectural tool for removing globals from our codebase. See **[PR13657](https://github.com/xbmc/xbmc/pull/13657)** for an overview of this system.

**[back to top](#table-of-contents)**

## 4. Step 1: Identify and remove the instance

Identify if the service is instantiated as a global or singleton. Remove this instance: **[Remove CStereoscopicsManager instance](https://github.com/garbear/xbmc/commit/35dc0e9~11)**.
Singletons usually have private constructors. Make all constructors public.

**[back to top](#table-of-contents)**

## 5. Step 2: Add instance to CServiceManager

First, identify if lines of code are listed in alphabetical or chronological order. Add the instance and try to match the ordering. In this case, services in CServiceManager are roughly in chronological order: **[Add CStereoscopicsManager instance to CServiceManager](https://github.com/garbear/xbmc/commit/35dc0e9~10)**.

**[back to top](#table-of-contents)**

## 6. Step 3: Move initialization and deinitialization to CServiceManager

This part is tricky. Identify the closest stage where the instance is initialized and move it to CServiceManager: **[Move CStereoscopicsManager init/deinit to CServiceManager](https://github.com/garbear/xbmc/commit/35dc0e9~9)**.

In this case, it was pretty easy. Init occurred right before Stage 3, so we move it into CServiceManager at the top of Stage 3.

Notice how deinit is done at the same relative place in the opposite order.

Before, CStereoscopicsManager was never deinitialized, so adding a deinit step in CServiceManager might cause crashes on shutdown. Regardless, we must be strict in CServiceManager, so we'll have to deal with shutdown crashes if any arise.

**[back to top](#table-of-contents)**

## 7. Step 4: Replace local dependencies with references

A dependency is the use of a variable in another part of Kodi. These can be identified by searching StereoscopicsManager.cpp for all globals (starting with `g_`) and external services (starting with `CServiceBroker::`).

Here is a list of all dependencies in StereoscopicsManager.cpp:

* `CServiceBroker::GetSettings()`
* `CServiceBroker::GetDataCacheCore()`
* `CServiceBroker::GetRenderSystem()`
* `g_advancedSettings`
* `g_windowManager`
* `g_localizeStrings`
* `g_graphicsContext`
* `g_application`

We'll ignore the `g_` globals and Render System for now and focus on removing the first two CServiceBroker ones. If you're feeling adventurous, you can recursively kill the globals and singletons you come across. **[PR13443](https://github.com/xbmc/xbmc/pull/13443)** is my record of 4 in a single PR :)

### 7.1. Step 4.1 - Settings

Because CStereoscopicsManager now belongs to CServiceManager, we can remove the use of CServiceBroker by passing references to the constructor upon creation. For example, here we replace access to CSettings with a reference that comes from CServiceManager: **[Replace global access to Settings with a reference](https://github.com/garbear/xbmc/commit/35dc0e9~8)**.

Note that the reference was placed first in the list of private variables. Some helpful comments were added so future authors will know where to add new variables.

### 7.2. Step 4.2 - Data Cache Core

Next, we do the same thing with CDataCacheCore. Notice that we place the new reference on the next line so we can scale to handle lots of dependencies with a clean git history: **[Replace global access to Data Cache Core with reference](https://github.com/garbear/xbmc/commit/35dc0e9~7)**.

**[back to top](#table-of-contents)**

## 8. Step 5: Make service registration local

Now we begin removing external access to the service. Move all service registration to the constructor, using the references we just exposed: **[Make service registration local](https://github.com/garbear/xbmc/commit/35dc0e9~6)**.

**[back to top](#table-of-contents)**

## 9. Step 6: Const-correctness and static cleanup

Take this opportunity to fix non-const and non-static functions:

* **[Make functions const-correct and static](https://github.com/garbear/xbmc/commit/35dc0e9~5)**
* **[Make functions private that are only used internally](https://github.com/garbear/xbmc/commit/35dc0e9~4)**

Calls to these functions are made throughout the codebase. Because we're touching all these calls, we can take this opportunity to propagate const-correctness and static-correctness through the codebase.

**[back to top](#table-of-contents)**

## 10. Step 7: Replace external uses with CServiceBroker calls

The final step is to grep the codebase and make all the substitutions required: **[Replace uses of singleton with CServiceBroker](https://github.com/garbear/xbmc/commit/35dc0e9~3)**.

Notice how our const-correctness and static-correctness are extended almost everywhere.

Congratulations! Global killed.

**[back to top](#table-of-contents)**

## 11. Step 8: Cleanup

Some code in Kodi goes back 17 years, long before our current style guide was in place. If you touch code that doesn't conform to our style, it's courteous to clean up after yourself.

It's important that all code cleanup is separate from functional changes. Cleanup commits should be grouped together at the very beginning or very end of the PR. They should be marked as such in the commit message.

I've performed the following cleanup:

* **[Cleanup: Replace C-style casts with C++ style casts](https://github.com/garbear/xbmc/commit/35dc0e9~2)**
* **[Cleanup: Fix early return](https://github.com/garbear/xbmc/commit/35dc0e9~1)**
* **[Cleanup: Whitespace improvements](https://github.com/garbear/xbmc/commit/35dc0e9)**

**[back to top](#table-of-contents)**

## 12. Step 9: Submit your PR

The commits in your PR should look like this one, except steps 1-7 should be squashed into a single commit. My steps were for demonstration purposes, and compilation breaks until all 7 are committed. Kodi should always compile between commits to aid in bisecting problems.

**[back to top](#table-of-contents)**

## 13. Appendix: Dealing with the human aspect

Most globals were added in a time before our technomancers gifted us with information on what good architecture looks like. This doesn't mean the original authors wrote "crappy" code, just that they did the best with what they were given.

As a result, globals are pervasive and dependencies are tangled and, in the nightmare cases, circular. It is almost guaranteed that your hard work will break something for someone else. Fortunately, you'll find most devs are understanding of this.

This can daisy-chain, because a fix for your breakage can cause even more breakage. If you feel any author of downstream breakage gets undue blame, stick up for them as other devs were understanding for you.

**[back to top](#table-of-contents)**

