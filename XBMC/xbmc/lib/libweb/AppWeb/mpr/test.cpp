///
///	@file 	test.cpp
/// @brief 	Embedded Test Unit Framework (EUnit)
///	@overview This file implements the EUnit test framework. 
//
////////////////////////////////// Copyright ///////////////////////////////////
//
//	@copy	default
//	
//	Copyright (c) Mbedthis Software LLC, 2003-2007. All Rights Reserved.
//	
//	This software is distributed under commercial and open source licenses.
//	You may use the GPL open source license described below or you may acquire 
//	a commercial license from Mbedthis Software. You agree to be fully bound 
//	by the terms of either license. Consult the LICENSE.TXT distributed with 
//	this software for full details.
//	
//	This software is open source; you can redistribute it and/or modify it 
//	under the terms of the GNU General Public License as published by the 
//	Free Software Foundation; either version 2 of the License, or (at your 
//	option) any later version. See the GNU General Public License for more 
//	details at: http://www.mbedthis.com/downloads/gplLicense.html
//	
//	This program is distributed WITHOUT ANY WARRANTY; without even the 
//	implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
//	
//	This GPL license does NOT permit incorporating this software into 
//	proprietary programs. If you are unable to comply with the GPL, you must
//	acquire a commercial license to use this software. Commercial licenses 
//	for this software and support services are available from Mbedthis 
//	Software at http://www.mbedthis.com 
//	
//	@end
//
/////////////////////////////////// Includes ///////////////////////////////////

#include	"test.h"

////////////////////////////// Forward Declarations ////////////////////////////

#if BLD_FEATURE_MULTITHREAD
static void		runWrapper(void *data, MprThread *tp);
#else
static void		runWrapper(void *data, void *tp);
#endif

static void catchSignal(int signo, siginfo_t *info, void *arg);
static void initSignals();

////////////////////////////////////////////////////////////////////////////////
//
//	FUTURE -- factory here
//

static MprTestSession	**sessions;

#if BLD_FEATURE_LOG
static MprLogToFile		*logger;
#endif

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////// MprTestFailure ////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

MprTestFailure::MprTestFailure(char *file, int line, char *message)
{
	this->file = mprStrdup(file);
	this->line = line;
	this->message = mprStrdup(message);
}

////////////////////////////////////////////////////////////////////////////////

MprTestFailure::~MprTestFailure()
{
	mprFree(file);
	mprFree(message);
}

////////////////////////////////////////////////////////////////////////////////
///////////////////////////////// MprTestResult ////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

MprTestResult::MprTestResult()
{
	continueOnFailures = 0;
	debugOnFailures = 0;
	singleStep = 0;
	start = mprGetTime(0);
	successes = new MprHashTable(211);
	testCount = 0;
	testFailedCount = 0;
	verbose = 0;

#if BLD_FEATURE_MULTITHREAD
	activeThreadCount = 0;
	mutex = new MprMutex();
#endif
}

////////////////////////////////////////////////////////////////////////////////

MprTestResult::~MprTestResult()
{
	MprTestFailure		*fp, *nextFp;
	MprTestListener	*lp, *nextLp;

	lock();
	lp = (MprTestListener*) listeners.getFirst();
	while (lp) {
		nextLp = (MprTestListener*) listeners.getNext(lp);
		listeners.remove(lp);
		if (strcmp(lp->getName(), "__default__") == 0) {
			delete lp;
		}
		lp = nextLp;
	}
	
	fp = (MprTestFailure*) failures.getFirst();
	while (fp) {
		nextFp = (MprTestFailure*) failures.getNext(fp);
		failures.remove(fp);
		delete fp;
		fp = nextFp;
	}

	delete successes;
#if BLD_FEATURE_MULTITHREAD
	delete mutex;
#endif
}

////////////////////////////////////////////////////////////////////////////////

bool MprTestResult::assertTrue(MprTest *test, char *file, int line, 
	bool success, char *testCode) 
{
	if (! success) {
		if (getDebugOnFailures()) {
			mprBreakpoint(0, 0, 0);
		}
		addFailure(new MprTestFailure(file, line, testCode));
		if (test->getFailureCount() == 0) {
			lock();
			testFailedCount++;
			unlock();
		}
		test->adjustFailureCount(1);
	} else {
		addSuccess(test);
	}
	return success;
}

////////////////////////////////////////////////////////////////////////////////

void MprTestResult::addFailure(MprTestFailure *failure) 
{
	lock();
	failures.insert(failure);
	unlock();
}

////////////////////////////////////////////////////////////////////////////////

void MprTestResult::addSuccess(MprTest *test)
{
	if (verbose > 1) {
		lock();
		successes->insert(new MprHashEntry(test->getName()));
		unlock();
	}
}

////////////////////////////////////////////////////////////////////////////////

bool MprTestResult::getContinueOnFailures() 
{ 
	return continueOnFailures; 
}

////////////////////////////////////////////////////////////////////////////////

bool MprTestResult::getDebugOnFailures() 
{ 
	return debugOnFailures; 
}

////////////////////////////////////////////////////////////////////////////////

int MprTestResult::getFailureCount() 
{ 
	return failures.getNumItems(); 
}

////////////////////////////////////////////////////////////////////////////////

MprList *MprTestResult::getFailureList() 
{ 
	return &failures; 
}

////////////////////////////////////////////////////////////////////////////////

MprTestFailure* MprTestResult::getFirstFailure() 
{ 
	return (MprTestFailure*) failures.getFirst(); 
}

////////////////////////////////////////////////////////////////////////////////

int MprTestResult::getVerbose() 
{ 
	return verbose; 
}

////////////////////////////////////////////////////////////////////////////////

int MprTestResult::report() 
{
	MprTestFailure		*tfp;
	MprHashEntry			*sp;
	MprTestListener		*lp;

	lock();
	lp = (MprTestListener*) listeners.getFirst();
	while (lp) {
		if (verbose) {
			lp->trace("\n");
		}
		lp->results("%d tests completed, %d test(s) failed\n", 
			testCount, testFailedCount);

		//
		// Do failure report
		//
		tfp = (MprTestFailure*) failures.getFirst();
		while (tfp) {
			if (tfp->line < 0) {
				lp->results("Failure in %s. Assertion: \"%s\"\n", 
					tfp->file, tfp->message);
			} else {
				lp->results("Failure at line %d in %s. Assertion: \"%s\"\n", 
					tfp->line, tfp->file, tfp->message);
			}
			tfp = (MprTestFailure*) failures.getNext(tfp);
		}

		//
		// Do success report (FUTURE -- this may be incomplete. Doen't include
		//	failed tests.
		//
		sp = (MprHashEntry*) successes->getFirst();
		while (sp) {
			lp->results("Ran test: %s\n", sp->getKey());
			sp = (MprHashEntry*) successes->getNext(sp);
		}
		
		if (verbose > 1) {
			lp->results("Elapsed time: %5.2f\n", 
				(mprGetTime(0) - start) / 1000.0);
		}
		lp->results("\n"); 
		lp = (MprTestListener*) listeners.getNext(lp);
	}
	unlock();
	return (failures.getNumItems() > 0) ? 1 : 0;
}

////////////////////////////////////////////////////////////////////////////////

void MprTestResult::setDebugOnFailures(bool on) 
{ 
	debugOnFailures = on; 
}

////////////////////////////////////////////////////////////////////////////////

void MprTestResult::setContinueOnFailures(bool on) 
{ 
	continueOnFailures = on; 
}

////////////////////////////////////////////////////////////////////////////////

void MprTestResult::setSingleStep(bool on) 
{ 
	singleStep = on; 
}

////////////////////////////////////////////////////////////////////////////////

void MprTestResult::adjustThreadCount(int adj)
{
	lock();
	activeThreadCount += adj;
	unlock();
}

////////////////////////////////////////////////////////////////////////////////

int MprTestResult::getThreadCount()
{
	return activeThreadCount;
}

////////////////////////////////////////////////////////////////////////////////

int MprTestResult::getListenerCount()
{
	return listeners.getNumItems();
}

////////////////////////////////////////////////////////////////////////////////

void MprTestResult::adjustTestCount(bool success, int adj)
{
	MprTestListener	*lp;

	lock();
	if (verbose) {
		lp = (MprTestListener*) listeners.getFirst();
		while (lp) {
			if (success) {
				lp->trace(".");
			} else {
				lp->trace("!");
			}
			if ((testCount % 50) == 49) {
				lp->trace(" (%d, %d)\n", testCount + 1, testFailedCount);
			}
			lp = (MprTestListener*) listeners.getNext(lp);
		}
	}
	testCount += adj;
	unlock();
}

////////////////////////////////////////////////////////////////////////////////

int MprTestResult::getTestCount()
{
	return testCount;
}

////////////////////////////////////////////////////////////////////////////////

void MprTestResult::setVerbosity(int v)
{
	verbose = v;
}

////////////////////////////////////////////////////////////////////////////////

bool MprTestResult::getSingleStep()
{
	return singleStep;
}

////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////// MprTest ////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

MprTest::MprTest(const char *name)
{
	condWakeFlag = 0;
	cond2WakeFlag = 0;
	fn = 0;
	level = MPR_BASIC;
	this->name = mprStrdup(name);
	failureCount = 0;
	success = 1;
	session = 0;
#if BLD_FEATURE_MULTITHREAD
	mutex = new MprMutex();
	cond = new MprCond();
	cond2 = new MprCond();
#endif
}

////////////////////////////////////////////////////////////////////////////////

MprTest::~MprTest()
{
	lock();
	mprFree(name);
#if BLD_FEATURE_MULTITHREAD
	delete cond;
	delete cond2;
	delete mutex;
#endif
}

////////////////////////////////////////////////////////////////////////////////

MprTestLevel MprTest::getLevel()
{
	return level;
}

////////////////////////////////////////////////////////////////////////////////

int MprTest::init(MprTestResult *rp)
{
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

void MprTest::term(MprTestResult *rp)
{
}

////////////////////////////////////////////////////////////////////////////////

int MprTest::classInit(MprTestResult *rp)
{
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

void MprTest::classTerm(MprTestResult *rp)
{
}

////////////////////////////////////////////////////////////////////////////////

void MprTest::setSession(MprTestSession *sp)
{
	session = sp;
}

////////////////////////////////////////////////////////////////////////////////

void MprTest::adjustFailureCount(int adj)
{
	lock();
	failureCount += adj;
	success = 0;
	unlock();
}

////////////////////////////////////////////////////////////////////////////////

int MprTest::getFailureCount()
{
	return failureCount;
}

////////////////////////////////////////////////////////////////////////////////

bool MprTest::isSuccess()
{
	return success;
}

////////////////////////////////////////////////////////////////////////////////

char *MprTest::getName()
{
	return name;
}

////////////////////////////////////////////////////////////////////////////////

void MprTest::reset()
{
	success = 1;
	condWakeFlag = 0;
	cond2WakeFlag = 0;

#if BLD_FEATURE_MULTITHREAD
	//
	//	If a previous test failed, a cond var may be still signalled. So we
	//	MUST recreate
	//
	delete cond;
	cond = new MprCond();
	delete cond2;
	cond2 = new MprCond();
#endif
}

////////////////////////////////////////////////////////////////////////////////

bool MprTest::waitForTest(int timeout)
{
	int		mark;

	mark = mprGetTime(0);
	if (mprGetDebugMode()) {
		timeout = INT_MAX - mark - timeout - 1;
	}

#if BLD_FEATURE_MULTITHREAD
	if (session->isRunningEventsThread()) {
		if (cond->waitForCond(timeout) < 0) {
			mprLog(1, "wait timeout %d, on test %s\n", timeout, getName());
			return 0;
		}

	} else 
#endif
	{
		//
		//	Use a short-nap here as complete may get set after testing at 
		//	the top of the loop and we may do a long wait for an event 
		//	that has already happened.
		//
		while (! condWakeFlag && mprGetTime(0) < (mark + timeout)) {
			session->getMprp()->serviceEvents(1, MPR_TEST_POLL_NAP);
		}
		if (condWakeFlag == 0) {
			mprLog(1, "wait2 timeout %d, on test %s\n", timeout, getName());
			return 0;
		}
		condWakeFlag = 0;
	}
	return 1;
}

////////////////////////////////////////////////////////////////////////////////

void MprTest::signalTest()
{
	condWakeFlag = 1;
#if BLD_FEATURE_MULTITHREAD
	if (session->isRunningEventsThread()) {
		cond->signalCond();
	}
#endif
}

////////////////////////////////////////////////////////////////////////////////

bool MprTest::waitForTest2(int timeout)
{
	int		mark;

	mark = mprGetTime(0);
#if BLD_FEATURE_MULTITHREAD
	if (session->isRunningEventsThread()) {
		if (cond2->waitForCond(timeout) < 0) {
			mprLog(1, "wait2 timeout %d, on test %s\n", timeout, getName());
			return 0;
		}
	} else 
#endif
	{
		//
		//	Use a short-nap here as complete may get set after testing at 
		//	the top of the loop and we may do a long wait for an event 
		//	that has already happened.
		//
		while (! cond2WakeFlag && mprGetTime(0) < (mark + timeout)) {
			session->getMprp()->serviceEvents(1, MPR_TEST_POLL_NAP);
		}
		if (cond2WakeFlag == 0) {
			mprLog(1, "wait2 timeout %d, on test %s\n", timeout, getName());
			return 0;
		}
		cond2WakeFlag = 0;
	}
	return 1;
}

////////////////////////////////////////////////////////////////////////////////

void MprTest::signalTest2()
{
	cond2WakeFlag = 1;
#if BLD_FEATURE_MULTITHREAD
	if (session->isRunningEventsThread()) {
		cond2->signalCond();
	}
#endif
}

////////////////////////////////////////////////////////////////////////////////

char **MprTest::getArgv()
{
	return session->getArgv();
}

////////////////////////////////////////////////////////////////////////////////

int MprTest::getArgc()
{
	return session->getArgc();
}

////////////////////////////////////////////////////////////////////////////////

int MprTest::getFirstArg()
{
	return session->getFirstArg();
}

////////////////////////////////////////////////////////////////////////////////
///////////////////////////////// MprTestCase //////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

MprTestCase::MprTestCase(const char *name, MprTestProc fn, MprTestLevel level) :
	MprTest(name)
{
	this->fn = fn;
	this->level = level;
}

////////////////////////////////////////////////////////////////////////////////

MprTestCase::~MprTestCase()
{
}

////////////////////////////////////////////////////////////////////////////////

void MprTestCase::run(MprTestResult *result, int level)
{
	int		pass = 0;

	do {
		group->reset();
		group->setResult(result);
		if (result->getSingleStep()) {
			mprPrintf("Run test %s: ", name);
			getchar();
		}

		mprLog(3, "%x: Running %s\n", this, name);

		result->setTestAgain(false);

		(group->*fn)(result);
		mprLog(8, "%x: AFTER Running %s\n", this, name);

		result->adjustTestCount(success, 1);
		if (!group->isSuccess()) {
			adjustFailureCount(1);
			if (result->getTestAgain()) {
				mprLog(3, "%x: Test %s - %d: FAILED\n", this, pass, name);
			} else {
				mprLog(3, "%x: Test %s: FAILED\n", this, name);
			}
		} else {
			if (result->getTestAgain()) {
				mprLog(3, "%x: Test %s - %d: PASSED\n", this, pass, name);
			} else {
				mprLog(3, "%x: Test %s: PASSED\n", this, name);
			}
		}
		pass++;
	} while (result->getTestAgain());
}

////////////////////////////////////////////////////////////////////////////////

void MprTestCase::setGroup(MprTestGroup *grp)
{
	group = grp;
}

////////////////////////////////////////////////////////////////////////////////
///////////////////////////////// MprTestGroup /////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

MprTestGroup::MprTestGroup(const char *name) : MprTest(name)
{
	result = 0;
}

////////////////////////////////////////////////////////////////////////////////

MprTestGroup::~MprTestGroup()
{
	MprTest	*tp, *nextTp;

	tp = (MprTest*) testList.getFirst();
	while (tp) {
		nextTp = (MprTest*) testList.getNext(tp);
		testList.remove(tp);
		delete tp;
		tp = nextTp;
	}
}

////////////////////////////////////////////////////////////////////////////////

void MprTestGroup::add(MprTestGroup *group, MprTestLevel level)
{
	testList.insert(group);
}

////////////////////////////////////////////////////////////////////////////////

void MprTestGroup::add(char *name, MprTestProc fn, MprTestLevel level) 
{
	MprTestCase	*testCase;

	testCase = new MprTestCase(name, fn, level);
	testList.insert(testCase);
	testCase->setGroup(this);
}

////////////////////////////////////////////////////////////////////////////////

MprTestResult *MprTestGroup::getResult() 
{ 
	return result; 
}

////////////////////////////////////////////////////////////////////////////////

int MprTestGroup::init(MprTestResult *rp)
{
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

void MprTestGroup::run(MprTestResult *result, int recurseDepth) 
{
	MprStringList	*groups;
	MprStringData	*dp;
	MprTest			*tp;
	int				count, runTest, testsRun;

	if (init(result) < 0) {
		adjustFailureCount(1);
		return;
	}

	testsRun = 0;
	tp = (MprTest*) testList.getFirst();
	while (tp) {
		runTest = 1;
		if (tp->getLevel() > session->getSessionLevel()) {
			runTest = 0;
		}

		groups = session->getTestGroups();
		if (recurseDepth == 0 && groups->getNumItems() > 0) {
			dp = (MprStringData*) groups->getFirst();
			while (dp) {
				if (strcmp(dp->string, tp->getName()) == 0) {
					break;
				}
				dp = (MprStringData*) groups->getNext(dp);
			}
			if (dp == 0) {
				runTest = 0;
			}
		}

		if (runTest) {
			count = result->getFailureCount();
			if (count > 0 && !result->getContinueOnFailures()) {
				term(result);
				return;
			}
			if (result->getSingleStep() || result->getVerbose() > 1) {
				mprPrintf("Running test %s.%s: \n", name, tp->getName());
			}
			tp->run(result, recurseDepth + 1);
			testsRun++;
			if (!tp->isSuccess()) {
				adjustFailureCount(1);
			}
		}
		tp = (MprTest*) testList.getNext(tp);
	}
	if (testsRun == 0) {
		mprPrintf("\nWARNING: "
			"Group \"%s\" has no tests to run at this level.\n", 
			this->getName());
	}
	term(result);
}

////////////////////////////////////////////////////////////////////////////////

void MprTestGroup::setResult(MprTestResult *rp) 
{ 
	result = rp; 
}

////////////////////////////////////////////////////////////////////////////////

void MprTestGroup::term(MprTestResult *rp)
{
}

////////////////////////////////////////////////////////////////////////////////

void MprTestGroup::setSession(MprTestSession *sp) 
{
	MprTest	*tp;

	session = sp;

	tp = (MprTest*) testList.getFirst();
	while (tp) {
		tp->setSession(sp);
		tp = (MprTest*) testList.getNext(tp);
	}
}

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////// MprTestSession ////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

MprTestSession::MprTestSession(char *name) : MprTestGroup(name)
{
	mpr = 0;
	iterations = 1;
	numThreads = 1;
	argc = 0;
	argv = 0;
	poolThreads = 0;
	needEventsThread = 0;
	verbose = 0;
	session = this;
	sessionLevel = MPR_BASIC;
	testGroups = new MprStringList();
}

////////////////////////////////////////////////////////////////////////////////

MprTestSession::~MprTestSession()
{
	delete testGroups;
}

////////////////////////////////////////////////////////////////////////////////

void MprTestSession::cloneSettings(MprTestSession *master)
{
	MprStringData	*dp;

	mpr = master->mpr;
	iterations = master->iterations;
	needEventsThread = master->needEventsThread;
	numThreads = master->numThreads;
	poolThreads = master->poolThreads;
	sessionLevel = master->sessionLevel;
	argv = master->argv;
	argc = master->argc;
	verbose = master->verbose;

	dp = (MprStringData*) master->testGroups->getFirst();
	while (dp) {
		testGroups->insert(dp->string);
		dp = (MprStringData*) master->testGroups->getNext(dp);
	}
}

////////////////////////////////////////////////////////////////////////////////

int MprTestSession::init()
{
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

void MprTestSession::term()
{
}

////////////////////////////////////////////////////////////////////////////////

int MprTestSession::initializeClasses(MprTestResult *result)
{
	MprTest	*tp;

	setResult(result);

	tp = (MprTest*) testList.getFirst();
	while (tp) {
		if (tp->getLevel() <= sessionLevel) {
			if (tp->classInit(result) < 0) {
				mprFprintf(MPR_STDERR, "Can't start test %s\n", tp->getName());
				return -1;
			}
		}
		tp = (MprTest*) testList.getNext(tp);
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

void MprTestSession::terminateClasses(MprTestResult *result)
{
	MprTest	*tp;

	tp = (MprTest*) testList.getFirst();
	while (tp) {
		if (tp->getLevel() <= sessionLevel) {
			tp->classTerm(result);
		}
		tp = (MprTest*) testList.getNext(tp);
	}
}

////////////////////////////////////////////////////////////////////////////////

void MprTestResult::addListener(MprTestListener *lp)
{
	lock();
	listeners.insert(lp);
	unlock();
}

////////////////////////////////////////////////////////////////////////////////

int MprTestSession::getIterations()
{
	return iterations;
}

////////////////////////////////////////////////////////////////////////////////

bool MprTestSession::isRunningEventsThread()
{
	return needEventsThread;
}

////////////////////////////////////////////////////////////////////////////////

Mpr *MprTestSession::getMprp()
{
	return mpr;
}

////////////////////////////////////////////////////////////////////////////////

char **MprTestSession::getArgv()
{
	return argv;
}

////////////////////////////////////////////////////////////////////////////////

int MprTestSession::getArgc()
{
	return argc;
}

////////////////////////////////////////////////////////////////////////////////

int MprTestSession::getFirstArg()
{
	return firstArg;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Main test runner
// 

int MprTestSession::setupTests(MprTestResult *result, Mpr *mpr, int argc, 
	char *argv[], char *switches)
{
	char			switchBuf[80];
	char			*programName, *argp;
	int				errflg, c, i, l;
#if BLD_FEATURE_LOG
	char			*logSpec;
#endif

	this->mpr = mpr;
	programName = mprGetBaseName(argv[0]);
	errflg = 0;
#if BLD_FEATURE_LOG
	logSpec = "stdout:1";
	logger = new MprLogToFile();
#endif

	switchBuf[0] = '\0';
	mprStrcat(switchBuf, sizeof(switchBuf), 0, "cDeg:i:l:n:msT:t:v?", 
		switches, (void*) 0);
	MprCmdLine	cmdLine(argc, argv, switchBuf);

	while ((c = cmdLine.next(&argp)) != EOF) {
		switch(c) {
		case 'c':
			result->setContinueOnFailures(1);
			break;

		case 'D':
			mprSetDebugMode(1);
			result->setDebugOnFailures(1);
			break;

		case 'e':
			needEventsThread = 1;
			break;

		case 'g':
			testGroups->parse(argp);
			break;

		case 'i':
			iterations = atoi(argp);
			break;

		case 'l':
#if BLD_FEATURE_LOG
			logSpec = argp;
#endif
			break;

		case 'n':
			l = atoi(argp);
			if (l == 0) {
				sessionLevel = MPR_BASIC;
			} else if (l == 1) {
				sessionLevel = MPR_THOROUGH;
			} else {
				sessionLevel = MPR_DEDICATED;
			}
			break;

		case 'm':
			mprRequestMemStats(1);
			break;

		case 's':
			result->setSingleStep(1);
			break;

		case 't':
			i = atoi(argp);
			if (i <= 0 || i > 100) {
				mprFprintf(MPR_STDERR, "%s: Bad number of threads (0-100)\n", 
					programName);
				exit(2);
			
			}
#if BLD_FEATURE_MULTITHREAD
			numThreads = i;
#endif
			break;

		case 'T':
#if BLD_FEATURE_MULTITHREAD
			poolThreads = atoi(argp);
#endif
			break;

		case 'v':
			verbose++;
			break;

		default:
			//
			//	Ignore args we don't understand
			//
			break;

		case '?':
			errflg++;
			break;
		}
	}
	if (errflg) {
		mprFprintf(MPR_STDERR, 
			"usage: %s [-cDemsv] [-g groups] [-i iterations] "
			"[-l logSpec] [-n testLevel] [-T poolThreads] [-t threads]\n", 
			programName);
		exit(2);
	}

#if !BLD_FEATURE_MULTITHREAD
	needEventsThread = 0;
#endif

#if BLD_FEATURE_LOG
	mpr->addListener(logger);
	mpr->setLogSpec(logSpec);
#endif

	initSignals();

	this->argc = argc;
	this->argv = argv;
	this->firstArg = cmdLine.firstArg();

#if BLD_FEATURE_MULTITHREAD
	mpr->setMaxPoolThreads(poolThreads);
#endif
	if (mpr->start(needEventsThread ? MPR_SERVICE_THREAD : 0) < 0) {
		return MPR_ERR_CANT_INITIALIZE;
	}

	result->adjustThreadCount(numThreads);
	result->setVerbosity(verbose);
	if (result->getListenerCount() == 0) {
		result->addListener(new MprTestListener("__default__"));
	}

	if (verbose) {
		mprFprintf(MPR_STDOUT, 
			"Testing: iterations %d, threads %d, pool %d, service thread %d\n", 
			iterations, numThreads, poolThreads, needEventsThread);
	}

	//
	//	Use current session object for the main thread
	//
	sessions = (MprTestSession**) mprMalloc(sizeof(MprTestSession*) * 
		numThreads);
	sessions[0] = this;
	if (sessions[0]->initializeClasses(result) < 0) {
		exit(3);
	}

#if BLD_FEATURE_MULTITHREAD
	//
	//	Now clone this session object for all other threads
	//
	for (i = 1; i < numThreads; i++) {
		char tName[64];
		sessions[i] = this->newSession();
		sessions[i]->setResult(result);
		sessions[i]->cloneSettings(this);
		mprSprintf(tName, sizeof(tName), "test.%d", i);
	}
#endif
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Main test runner
// 

int MprTestSession::runTests(MprTestResult *result)
{
	int				i;
#if BLD_FEATURE_MULTITHREAD
	MprThread		*tp;
#endif

	for (i = 0; i < numThreads; i++) {
		if (sessions[i]->init() < 0) {
			return MPR_ERR_CANT_INITIALIZE;
		}
	}

#if BLD_FEATURE_MULTITHREAD
	//
	//	Now clone this session object for all other threads
	//
	for (i = 1; i < numThreads; i++) {
		char tName[64];
		mprSprintf(tName, sizeof(tName), "test.%d", i);
		tp = new MprThread(runWrapper, MPR_NORMAL_PRIORITY, sessions[i], tName);
		if (tp->start() < 0) {
			break;
		}
	}
#endif
	runWrapper(sessions[0], 0);

#if BLD_FEATURE_MULTITHREAD
	//
	//	Wait for all the threads to complete (simple but effective)
	//
	while (result->getThreadCount() > 1) {
		mprSleep(75);
	}
#endif
	sessions[0]->terminateClasses(result);

#if BLD_FEATURE_MULTITHREAD
	for (i = 1; i < numThreads; i++) {
		sessions[i]->term();
		delete sessions[i];
	}
#endif

	mprFree(sessions);

#if BLD_FEATURE_LOG
	mpr->logService->removeListener(logger);
	delete logger;
#endif
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

#if BLD_FEATURE_MULTITHREAD
void runWrapper(void *data, MprThread *threadp)
#else
void runWrapper(void *data, void *threadp)
#endif
{
	MprTestSession	*session;
	MprTestResult	*result;
	int				i;

#if TROUBLE
	//
	//	Skew the thread priorities a bit 
	//
	if (threadp) {
		//
		//	Under extreme load, this can cause a bit of thread starvation
		//	and cause some tests to timeout
		//
		threadp->setPriority(MPR_NORMAL_PRIORITY - 10 + (rand() % 20));
	}
#endif

	session = (MprTestSession*) data;
	session->setSession(session);
	result = session->getResult();

	for (i = 0; i < session->getIterations(); i++) {
		if (result->getFailureCount() > 0 &&
				!result->getContinueOnFailures()) {
			break;
		}
		if (threadp) {
			//
			//	Skew the threads a little
			//
			//		mprSleep(rand() % 500);
		}
		session->run(result, 0);
		//	mprPrintf("%s: iteration %d\n", mprGetCurrentThreadName(), i);
	}

#if BLD_FEATURE_MULTITHREAD
	if (threadp) {
		result->adjustThreadCount(-1);
	}
#endif
}

////////////////////////////////////////////////////////////////////////////////

void MprTestSession::setEventsThread()
{
	needEventsThread = 1;
}

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////// MprTestListener ///////////////////////////////
////////////////////////////////////////////////////////////////////////////////

MprTestListener::MprTestListener(char *name)
{
	this->name = mprStrdup(name);
}

////////////////////////////////////////////////////////////////////////////////

MprTestListener::~MprTestListener()
{
	mprFree(name);
}

////////////////////////////////////////////////////////////////////////////////

void MprTestListener::trace(char *fmt, ...)
{
	va_list	args;

	va_start(args, fmt);
	vfprintf(stdout, fmt, args);
	fflush(stdout);
	va_end(args);
}

////////////////////////////////////////////////////////////////////////////////

void MprTestListener::results(char *fmt, ...)
{
	va_list	args;

	va_start(args, fmt);
	vfprintf(stdout, fmt, args);
	fflush(stdout);
	va_end(args);
}

////////////////////////////////////////////////////////////////////////////////

char *MprTestListener::getName()
{
	return name;
}

////////////////////////////////////////////////////////////////////////////////

static void initSignals()
{
#if CYGWIN || LINUX || MACOSX || SOLARIS || VXWORKS || FREEBSD
	struct sigaction	act;

	memset(&act, 0, sizeof(act));
	act.sa_sigaction = catchSignal;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;

	sigaction(SIGQUIT, &act, 0);
	sigaction(SIGTERM, &act, 0);

	signal(SIGPIPE, SIG_IGN);
#endif
}

////////////////////////////////////////////////////////////////////////////////
//
//	Catch signals. Do a graceful shutdown.
//

static void catchSignal(int signo, siginfo_t *info, void *arg)
{
	mprLog(MPR_INFO, "Received signal %d\nExiting ...\n", signo);
	if (mprGetMpr()) {
		mprGetMpr()->terminate(1);
	}
}

////////////////////////////////////////////////////////////////////////////////

//
// Local variables:
// tab-width: 4
// c-basic-offset: 4
// End:
// vim: sw=4 ts=4 
//
