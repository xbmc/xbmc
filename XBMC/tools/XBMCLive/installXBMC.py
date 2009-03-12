#!/usr/bin/env python
"""
  "XBMC Live" installer
  V0.981 - 20090310
  Luigi Capriotti @2009

""" 

import commands
import tempfile
import os, re, sys, time
import subprocess
import random
import shutil
import statvfs
import optparse

gMinSizeMB = 1500

# For HDD installations
gFixedDiskMinSizeMB = 3500
gBootPartitionSizeMB = 2048
gSwapPartitionSizeMB = 512

gPermStorageFilename = "ext3fs.img"
gBootPartMountPoint = "/tmp/bootPart"
gLivePartMountPoint = "/tmp/livePart"

gDebugMode = 0
gInstallerLogFileName = None

def diskSizeKB(aVolume):
	diskusage = commands.getoutput('fdisk -l ' + aVolume + ' | grep "Disk ' + aVolume + '" | cut -f 5 -d " "').split('\n')
	nBytes = int(diskusage[len(diskusage)-1])
	return int(nBytes / 1024)

def freeSpaceMB(aPath):
#	stats = os.statvfs(aPath)
#	return (stats.f_bsize * stats.f_bavail)/1024
	stats = subprocess.Popen(["df", "-Pk", aPath], stdout=subprocess.PIPE).communicate()[0]
	return int(stats.splitlines()[1].split()[3])/1024

def readFile(the_file):
	f = file(the_file, 'r')
	content = f.read()
	f.close()
	return content

def writeFile(the_file, content):
	f = file(the_file, 'w')
	f.write(content)
	f.close

def writeLog(aLine):
	global gInstallerLogFileName
	
	time_now = time.strftime("[%H:%M:%S] ", time.localtime())
	f = file(gInstallerLogFileName, 'a')
	f.write(time_now + aLine + '\n')
	f.close

def getKernelParameter(token):
	aParam=None
	l=open('/proc/cmdline').readline().strip()
	for t in l.split():
		if t.startswith(token + '='):
			aParam = t.split('=',1)[1]
		
	return aParam

def findBootVolume(lookForRootFS):
	if not lookForRootFS:
		rootPartition = getKernelParameter('root')
		if rootPartition.startswith('UUID='):
			anUUID = rootPartition.split('=',1)[1]
			rootPartition = commands.getoutput('blkid | grep "' + anUUID + '" | cut -d ":" -f 1')
		return rootPartition

	bootMedia = getKernelParameter('boot')
	driveList = ["sr0","sr1","sr2","sr3"]
	if bootMedia == 'disk' or bootMedia == 'usb':
		driveList = ["sda","sdb","sdc","sdd","sde","sdf","sdg"]

	# Find the drive we booted from
	found = False
	for deviceNode in driveList:
		device = "/dev/" + deviceNode
		print "Checking device " + device + " for installation source..."
		if  os.path.exists(device):
			if  os.path.exists("/sys/block/" + deviceNode + "/removable"):
				if isRemovableDisk(device):
					if bootMedia == 'disk' or bootMedia == 'usb':
						mountDevice(device + "1", "", gBootPartMountPoint)
					else:
						mountDevice(device, "-o ro -t iso9660", gBootPartMountPoint)

					if os.path.exists(gBootPartMountPoint + "/rootfs.img"):
						print "Found Boot drive on " + device
						found = True

					umountDevice(gBootPartMountPoint)

		if found == True:
			break

		print device + " does not contain a rootfs."

	if found == False:
		device = None

	return device

def isRemovableDisk(aDevice):
	aDeviceNode = aDevice.split("/")[2]

	fName='/sys/block/' + aDeviceNode + '/removable'
	if os.path.exists(fName):
		return int(open(fName).readline().strip())
	return 0


def userChoice(prompt, charsGo, charsStop):
	bGoOn=0
	while 1:
		choice = raw_input(prompt)
		if choice == '':
			continue
		if choice in charsGo:
			bGoOn = choice
			break
		if choice in charsStop:
			break

	return bGoOn

def findDisks(bootDisk):
	availDisk = []
	
	while 1:
		found=0
		for deviceNode in ['sda','sdb','sdc','sdd','sde','sdf','sdg']:
			device = "/dev/" + deviceNode
			if bootDisk.find(device) >=0:
				print "Skipping boot device (" + device + ") ..."
			else:
				print "Checking device " + device +" ..."
	
				if os.path.exists(device):
					aFile = '/sys/block/' + deviceNode + '/size'
					if open(aFile).readline().strip() != "0" :
						availDisk.append(device)

						aString = "Found disk: " + device
						if isRemovableDisk(device):
							aString += " - Removable disk."
						print aString
						found=1

		if found == 1:
			break

		raw_input("No drives detected, connect a disk and press return...")
		print ""
		print "Scanning again disk drives..."
		print ""

	return availDisk

def chooseDisk(availableDrives):
	if len(availableDrives) == 0:
		print
		print "No disks"
		print
		return 0
	print ""
	print "Choose disk to use"
	print ""
	availChoices=""
	nChoice = 0
	for t in availableDrives:
		diskSizeMB = int(diskSizeKB(t))/1000
		nChoice += 1
		availChoices += str(nChoice)
		aString = "   " + str(nChoice) + ": " + t + " (" + str(diskSizeMB) + " MB)"

		if isRemovableDisk(t):
			aString += " - Removable disk"
		else:
			aString += " - FIXED disk"
		print aString

	print ""
	return  userChoice("Type the digit, or 0 to restart the procedure: ", availChoices, "0")

def runSilent(aCmdline):
	global gDebugMode

	if gDebugMode>0:
		writeLog("Running: " + aCmdline)
	process = subprocess.Popen(aCmdline, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
	stdout_value, stderr_value = process.communicate()
	retCode = process.returncode
	writeLog("Return code= " + str(retCode))
	if gDebugMode>0:
		writeLog("Results: StdOut=" + repr(stdout_value))
		writeLog("Results: StdErr=" + repr(stderr_value))

	if retCode != 0:
		raise Exception, str(retCode)
	return stdout_value

def partitionFormatDisk(device, bootPartSize, swapPartSize):
	global gDebugMode

	runSilent("dd if=/dev/zero of=" + device + " bs=512 count=2")
	runSilent("sync")

	if bootPartSize <=0:
		partEnd = ""
	else:
		partEnd = "+" + str(bootPartSize) + "M"

	strFdiskCommands = ["o","n","p","1","",partEnd,"t","c","a","1","w"]

	stdInFile = tempfile.NamedTemporaryFile()
	for line in strFdiskCommands:
		stdInFile.write(line + "\n")
	stdInFile.flush()

	runSilent("cat " + stdInFile.name + " | fdisk " + device)
	stdInFile.close()

	runSilent("mkfs.vfat -I -F 32 -n XBMCLive_" + str(random.randint(0, 999)) + " " + device + "1")

	if swapPartSize !=0:
		partEnd = "+" + str(swapPartSize) + "M"

		strFdiskCommands = ["n","p","2","",partEnd,"t","2","82","n","p","3","","","w"]

		stdInFile = tempfile.NamedTemporaryFile()
		for line in strFdiskCommands:
			stdInFile.write(line + "\n")
		stdInFile.flush()

		runSilent("cat " + stdInFile.name + " | fdisk " + device)
		stdInFile.close()

		runSilent("mkswap -c " + device +"2")
		runSilent("mkfs.ext3 " + device +"3")
	else:
		if bootPartSize >0:
			strFdiskCommands = ["n","p","2","","","w"]
			stdInFile = tempfile.NamedTemporaryFile()
			for line in strFdiskCommands:
				stdInFile.write(line + "\n")
			stdInFile.flush()
			runSilent("cat " + stdInFile.name + " | fdisk " + device)
			stdInFile.close()

			runSilent("mkfs.ext3 " + device +"2")

	runSilent("sync")
	if gDebugMode>0:
		runSilent("fdisk -l " + device)
		runSilent("mount")

def mountDevice(aDevice, mountOpts, aDirectory):
	if not os.path.exists(aDirectory):
		os.mkdir(aDirectory)
	try:
		runSilent("mount " + mountOpts + " " + aDevice + " " + aDirectory)
	except:
		print "Error mounting device: " + aDevice + " - Exiting."
		sys.exit(-1)

def umountDevice(aDirectory, removeMountPoint=True):
	runSilent("umount " + aDirectory)
	if removeMountPoint == True:
		os.rmdir(aDirectory)

def copySystemFiles(srcDirectory, dstDirectory):
	global gDebugMode
	# This one needs python 2.6, for future use
	# Do not copy storage file
	# shutil.copytree(srcDirectory, dstDirectory, ignore=shutil.ignore_patterns(gPermStorageFilename))

	if not os.path.exists(dstDirectory):
		os.mkdir(dstDirectory)

	if gDebugMode>0:
		runSilent("mount")
		runSilent("ls -aRl " + srcDirectory)
		runSilent("ls -aRl " + dstDirectory)

	for root, dirs, files in os.walk(srcDirectory):
		for file in files:
			# Do not copy storage file
			if file == gPermStorageFilename:
				continue
			if gDebugMode>0:
				writeLog("Copying file: " + file)
			if gDebugMode>10:
				if file.find("img") > 0:
					writeLog("DEBUG MODE = " + str(gDebugMode) + " : file skipped.")
					continue

			from_ = os.path.join(root, file)
			to_ = from_.replace(srcDirectory, dstDirectory, 1)
			to_directory = os.path.split(to_)[0]
			if not os.path.exists(to_directory):
				os.makedirs(to_directory)
			try:
				shutil.copyfile(from_, to_)
			except:
				print "Error copying file: " + file + " - check your media"
				continue

	if not os.path.exists(dstDirectory + "/" + "Config"):
		os.mkdir(dstDirectory + "/" + "Config")


def createPermanentStorageFile(aFileFName, aSizeMB):
	aCmdLine = "dd if=/dev/zero of=" + aFileFName + " bs=4M count=" + str(aSizeMB/4)
	runSilent(aCmdLine)
	runSilent("mkfs.ext3 -F " + aFileFName)
	return

def findUUID(aPartition):
	cmdLine = 'blkid | grep ' + aPartition + ' | cut -d " " -f 2 | cut -d "=" -f 2 | sed \'s/"//g' + "'"
	return runSilent(cmdLine)

def installGrub(bootDevice, dstDirectory):
	runSilent("grub-install --recheck  --force-lba --root-directory=" + dstDirectory + " " + bootDevice)

def modifyGrubMenu(menuFName, isaRemovableDrive, bootPartition):
	content = readFile(menuFName)

	if isaRemovableDrive:
		content = re.sub("boot=[a-z]*", "boot=usb", content)
	else:
		content = re.sub("boot=[a-z]*", "boot=disk", content)

		# Defaults to current GPU
		hasAMD = commands.getoutput("lspci -nn | grep 0300 | grep 1002")
		hasNVIDIA = commands.getoutput("lspci -nn | grep 0300 | grep 10de")

		gpuType=2
		if len(hasNVIDIA):
			gpuType=0
		if len(hasAMD):
			gpuType=1

		content = re.sub("default [0-9]", "default " + str(gpuType), content)
		content = re.sub("timeout [0-9]*", "timeout 1", content)
		content = "hiddenmenu\n" + content

		content = ("groot=UUID=" + findUUID(bootPartition) + "\n") + content

	writeFile(menuFName, content)

def changePasswords(liveRootDir):
	# Lock root account
	runSilent("passwd -l root")

	print "Please set a new password for user 'xbmc':"
	while 1:
		retcode = subprocess.call(["passwd", "xbmc"])
		if retcode >= 0:
			break

	shutil.copyfile("/etc/shadow", liveRootDir + "/Config/shadow")

def prepareFstab(liveRootDir, bootDevice, swapFileSize):
	# Prepare /etc/fstab
	content =  "unionfs     /               unionfs defaults      0 0\n"
	content += "proc        /proc           proc    defaults      0 0\n"
	
	if swapFileSize == 0:
		content += ("UUID=" + findUUID(bootDevice + "2") + "   /home           ext3    defaults,auto 0 0\n")
	else:
		content += ("UUID=" + findUUID(bootDevice + "2") + "   none            swap    sw,auto       0 0\n")
		content += ("UUID=" + findUUID(bootDevice + "3") + "  /home            ext3    defaults,auto 0 0\n")
	writeFile(liveRootDir + "/Config/fstab", content)


def prepareHomeDirectory(bootDevice, swapFileSize):
	homeMountPoint = "/tmp/homePart"

	if swapFileSize > 0:
		mountDevice(bootDevice + "3", "", homeMountPoint)
	else:
		mountDevice(bootDevice + "2", "", homeMountPoint)

	if not os.path.exists(homeMountPoint + "/xbmc"):
		os.mkdir(homeMountPoint + "/xbmc")

	if not os.path.exists(homeMountPoint + "/xbmc/Videos"):
		os.mkdir(homeMountPoint + "/xbmc/Videos")

	if not os.path.exists(homeMountPoint + "/xbmc/Pictures"):
		os.mkdir(homeMountPoint + "/xbmc/Pictures")

	if not os.path.exists(homeMountPoint + "/xbmc/Music"):
		os.mkdir(homeMountPoint + "/xbmc/Music")

	if not os.path.exists(homeMountPoint + "/xbmc/.xbmc"):
		os.mkdir(homeMountPoint + "/xbmc/.xbmc")

	if not os.path.exists(homeMountPoint + "/xbmc/.xbmc/userdata"):
		os.mkdir(homeMountPoint + "/xbmc/.xbmc/userdata")

	# Create a sources.xml referencing the above created directories
	content  = "<sources>"
	content += "    <video>"
	content += "        <default></default>"
	content += "        <source>"
	content += "            <name>Videos</name>"
	content += "            <path>/home/xbmc/Videos/</path>"
	content += "        </source>"
	content += "    </video>"
	content += "    <music>"
	content += "        <default></default>"
	content += "        <source>"
	content += "            <name>Music</name>"
	content += "            <path>/home/xbmc/Music/</path>"
	content += "        </source>"
	content += "    </music>"
	content += "    <pictures>"
	content += "        <default></default>"
	content += "        <source>"
	content += "            <name>Pictures</name>"
	content += "            <path>/home/xbmc/Pictures/</path>"
	content += "        </source>"
	content += "    </pictures>"
	content += "</sources>"

	writeFile(homeMountPoint + "/xbmc/.xbmc/userdata/sources.xml", content)

	runSilent("chown -R xbmc:xbmc " + homeMountPoint + "/xbmc")
	umountDevice(homeMountPoint)

def main():
	global gMinSizeMB
	global gFixedDiskMinSizeMB
	global gBootPartitionSizeMB
	global gSwapPartitionSizeMB
	global gPermStorageFilename
	global gBootPartMountPoint
	global gLivePartMountPoint
	global gInstallerLogFileName
	global gDebugMode

	parser = optparse.OptionParser()
	parser.add_option("-i", dest="isoFileName", help="Use specified ISO file as source for XBMC Live files")
	parser.add_option("-d", dest="debugFileName", help="Use specified file as debug log file")
	parser.add_option("-s", dest="skipFileCopy", action="store_true", help="Don't copy IMG files (debug helper)", default=False)
	parser.add_option("-c", dest="doNotShutdown", action="store_true", help="Do not perform a shutdown after execution", default=False)
	(cmdLineOptions, args) = parser.parse_args()

	if not cmdLineOptions.debugFileName == None:
		gInstallerLogFileName = cmdLineOptions.debugFileName
		gDebugMode = 1
		writeLog("-- Installer Start --")

	if cmdLineOptions.skipFileCopy == True:
		gDebugMode = 11

	if not cmdLineOptions.isoFileName == None:
		cmdLineOptions.doNotShutdown = True

		if not os.path.exists(cmdLineOptions.isoFileName):
			print "File: " + cmdLineOptions.isoFileName + " does not exist, exiting..."
			sys.exit(-1)

		cmdLine = 'file -b -i ' + cmdLineOptions.isoFileName
		if not commands.getoutput(cmdLine) == "application/x-iso9660-image":
			print "File: " + cmdLineOptions.isoFileName + " is not a valid ISO image, exiting..."
			sys.exit(-1)

	availableDisks = []

	while True:
		os.system('clear')
		print ""
		print "XBMC Live bootable disk creator"
		print "---------------------"
		print ""
		print "The procedure will create a XBMC Live bootable disk"
		print ""
		print "Requirement for USB flash disks: the disk must have at least " + str(gMinSizeMB) + " MB of capacity!"
		print "Requirement for fixed disks: the disk must have at least " + str(gFixedDiskMinSizeMB) + " MB of capacity!"
		print ""
		print "Select the disk you want to use."
		print "CAUTION: the process will erase all data on the specified disk drive!"
		print "CAUTION: this is an experimental tool, use at your own risk!"
		print ""
		raw_input("Press a key to continue, or Ctlr-C to exit.")
		
		print ""
		print "Identifying boot disk..."
		print ""
		bootVolume = findBootVolume( (cmdLineOptions.isoFileName == None) )
		if bootVolume == None:
			if cmdLineOptions.doNotShutdown == True:
				raw_input("Error locating boot media, exiting.")
				sys.exit(-1)

			raw_input("Error locating boot media, press a key to shutdown...")
			runSilent("shutdown -h now")
			runSilent("sleep 10")
			sys.exit(-1)

		print "Enumerating available disks..."
		print ""
		availableDisks = findDisks(bootVolume)

		diskIndex = chooseDisk(availableDisks)
		if diskIndex == 0:
			continue
		diskIndex = int(diskIndex) - 1

		diskSizeMB = diskSizeKB(availableDisks[diskIndex])/1024
		minSizeMB = gMinSizeMB
		if not isRemovableDisk(availableDisks[diskIndex]):
			minSizeMB = gFixedDiskMinSizeMB

		if diskSizeMB < minSizeMB:
			print ""
			print ""
			print "The selected disk is too small."
			if isRemovableDisk(availableDisks[diskIndex]):
				print "The miminum requirements for USB flash disks are: " + str(gMinSizeMB) + " MB of disk size"
			else:
				print "The miminum requirements for fixed disks are: " + str(gFixedDiskMinSizeMB) + " MB of disk size"
			print
			print "The selected disk has only " + str(diskSizeMB) + " MB of disk space."
			print ""
			print "Please ignore the selected disk and try again."
			print ""
			raw_input("Press a button to restart the procedure. ")
			continue

		if userChoice("Erasing disk " + availableDisks[diskIndex] + ", proceed (Y/N)? ","Yy","Nn") == 0:
			continue

		print "Partitioning & formatting disk..."

		if isRemovableDisk(availableDisks[diskIndex]):
			gBootPartitionSizeMB = -1
			gSwapPartitionSizeMB = 0

		partitionFormatDisk(availableDisks[diskIndex], gBootPartitionSizeMB, gSwapPartitionSizeMB)

		print "Mounting source and destination disks..."

		mountDevice(availableDisks[diskIndex] + "1", "", gLivePartMountPoint)

		if cmdLineOptions.isoFileName == None:
			mountDevice(bootVolume, "-t iso9660,vfat", gBootPartMountPoint)
		else:
			mountDevice(cmdLineOptions.isoFileName, "-o loop", gBootPartMountPoint)

		print "Copying system files - can take a while..."

		copySystemFiles(gBootPartMountPoint, gLivePartMountPoint)

		umountDevice(gBootPartMountPoint)

		print "Installing GRUB..."
		installGrub(availableDisks[diskIndex], gLivePartMountPoint)

		if not gDebugMode > 10:
			if not userChoice("Create a permanent system storage file (Y/N)? ","Yy","Nn") == 0:
				availSpace = freeSpaceMB(availableDisks[diskIndex] + "1")
				storageSize = (availSpace/10)*7
				if storageSize > 4000:
					storageSize = 4000
				print "Permanent system storage size = " + str(storageSize) + " MB, Please wait..."
				createPermanentStorageFile(gLivePartMountPoint + "/" + gPermStorageFilename, storageSize)

		modifyGrubMenu(gLivePartMountPoint + "/boot/grub/menu.lst", isRemovableDisk(availableDisks[diskIndex]), availableDisks[diskIndex] + "1")

		if not isRemovableDisk(availableDisks[diskIndex]):
			print "Applying system changes..."
			prepareFstab(gLivePartMountPoint, availableDisks[diskIndex], gSwapPartitionSizeMB)
			changePasswords(gLivePartMountPoint)
			prepareHomeDirectory(availableDisks[diskIndex], gSwapPartitionSizeMB)

		umountDevice(gLivePartMountPoint)

		print "All done!"
		print ""

		if cmdLineOptions.doNotShutdown == True:
			print "Exiting..."
			break

		if userChoice("Do you want to create another bootable disk (Y/N)? ","Yy","Nn") == 0:
			print "Shutting down the system in 5 seconds..."
			runSilent("sleep 5")
			runSilent("shutdown -h now")
			sys.exit(-1)


if __name__ == '__main__':
	# Make sure only root can run the script
	if os.getuid() != 0: 
		print "This script must be run as root." 
		sys.exit(-1)

	main()
