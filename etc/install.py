#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Fri Apr 30 09:45:00 2021

@author: espressjo
"""
from cfgentry import cfgentry
from os import listdir,getcwd
from os.path import join,isdir,isfile
from os import system
import os

USER = "spip"
BASEFOLDER = 'HxRG-SPIP' #the name of the cloned repository 
TARGET = 'HxRG-SPIP' #name of the binary

class get_path():
    def __init__(self):
        self.base_path = ''
        p = getcwd()
        self.bbpath = join("/opt",TARGET)
        if BASEFOLDER not in p:
            print("It seems the script is not runned where it should be. Exiting...")
            if "yes" not in input("Do you want to continue with the install? [yes/no]"):
                exit(0)
        _l = [];
        for n in p.strip().split("/"):
            _l.append(n)
            if n==BASEFOLDER:
                break
        l = "/".join(_l)
        self.hxrg = l
        print("Path: %s"%self.hxrg)
        
        self.hxrg_src = join(l,'src')
        self.hxrg_lib = join(l,'lib')
        self.hxrg_cfg = join(l,'cfg')
        self.hxrg_mcd = join(l,'mcd')
        self.uics_cfg = join(l,'UICS/conf')
       
        self.std_bin = join(self.bbpath,"bin")
        self.std_cfg = join(self.bbpath,"config")
        self.std_data = join(self.bbpath,"data")
        self.std_ref = join(self.bbpath,"cal")
        self.std_log = join(self.bbpath,"log")
        self.std_lib = join(self.bbpath,"lib")
        self.std_ui = join(self.bbpath,"UI")
        
        ls = [self.bbpath,self.std_bin,self.std_cfg,self.std_data,self.std_ref,self.std_log,self.std_ui,self.std_lib]
        for d in ls:
            if not isdir(d):
                if 'y' in input("%s does not exist. Do you want to create it? [yes/no]\n"%d):
                    if system("mkdir -p %s"%d)!=0:
                        print("It seems you do not have the write access. Manually create;\n\t%s"%d)
                        input("Press any key")
                        
class unloadlib(get_path):
    def __init__(self):
        #get distribution
        get_path.__init__(self)
        self.distro = self.get_distro()
        if 'ubuntu' not in self.distro and 'centos' not in self.distro:
            print("Only centOS or Ubuntu is supported.")
            exit(0)
        
    def get_distro(self):
        '''
        Return the linux distribution .
    
        Returns
        -------
        string
            Return the linux distro in lower case [ubuntu/centos].
    
        '''
        with open('/etc/os-release','r') as f:
            for line in f.readlines():
                if 'NAME' in line:
                    if 'centos' in line.lower():
                        return 'centos'
                    elif 'ubuntu' in line.lower():
                        return 'ubuntu'
                    else:
                        return ''
            return ''
        
    def untar_lib(self):
        '''
        Deploy the library. 

        Returns
        -------
        None.

        '''
        file = '%s.tar.xz'%join(self.hxrg_lib,self.distro)
        cmd = "cd %s && tar -xvf %s -C %s"%(self.hxrg_lib,file,self.std_lib)
        system(cmd)
        '''
        ls = [join(self.hxrg_lib,f) for f in listdir(self.hxrg_lib) if all(['.tar.xz' not in f,'.py' not in f])]
        for f in ls:
            system("cp %s %s"%(f,self.std_lib))
        '''
        
    def clean_lib_dir(self):
        '''
        Clean the library

        Returns
        -------
        None.

        '''
        ls = [join(self.std_lib,f) for f in listdir(self.std_lib)]
        if len(ls)!=0:
            for f in ls:
                system("rm -rf %s"%f)
        if isfile(join(self.hxrg_lib,'ubuntu.tar.xz')) or isfile(join(self.hxrg_lib,'centos.tar.xz')):
            ls = [join(self.hxrg_lib,f) for f in listdir(self.hxrg_lib) if all(['.py' not in f, '.tar.xz' not in f])]
            for f in ls:
                system("rm %s"%f)
        else:
            print("lib folder seem to be modified, re-clone the code.")
            exit(0)
class stdinstallation(get_path):
    def __init__(self):
        get_path.__init__(self)
        
    def set_config(self):
        #HxRG.init
        #first we create the actual config files
        system("cp %s %s"%(join(self.hxrg_cfg,"HxRG.cfg"),join(self.std_cfg,"HxRG.init") ))
        system("cp %s %s"%(join(self.hxrg_cfg,"spip.cfg"),join(self.std_cfg,"spip.conf") ))
        from sys import argv
        #mod_cfg_file must be called at least once
        
        self.mod_cfg_file("OBSERVATORY", "--")#default value, will be erase
        self.mod_cfg_file("OBSLOCATION", "--")   #default value, will be erase     
        self.mod_cfg_file("ASICSERIAL", "205")#default value, will be erase 
        self.mod_cfg_file("SCASERIAL", "19909")#default value, will be erase
        self.mod_cfg_file("MCDASIC", join(self.std_cfg,"HxRG_Main.mcd"))
        self.mod_cfg_file("MRF", join(self.std_cfg,"MACIE_Registers_Slow.mrf"))
        self.mod_cfg_file("PATHLOG", self.std_log)
        self.mod_cfg_file("PATH", self.bbpath)
        self.mod_cfg_file("SOFTV", "2.0")
        self.mod_cfg_file("PATHCFG", self.std_cfg)
        self.mod_cfg_file("USER", USER)
        
        #nirps.conf
        self.mod_cfg_file("DATAPATH", self.std_data,  cfgfile="spip.conf")
        self.mod_cfg_file("CALPATH", self.std_ref,  cfgfile="spip.conf")
        #self.mod_cfg_file("OPTMCD", join(self.std_cfg,'optimisation_18859_157_diff_18dB.mcd'),  cfgfile="nirps.conf")
        
        if '--defcfg' in argv:
            return
        print("Setting various config files")
        for q in ["OBSERVATORY","OBSLOCATION"]:
            answ = input("%s: "%q)
            if answ:
                self.mod_cfg_file(q, answ)
    def install(self):

        system("cp %s %s"%(join(self.hxrg_cfg,"cmd.cfg"),join(self.std_cfg,"cmd.conf") ))
        system("cp %s %s"%(join(self.uics_cfg,"state.conf"),self.std_cfg))
        if isfile(join(self.hxrg_mcd,'HxRG_Main.mcd')):
            system("cp %s %s"%(join(self.hxrg_mcd,"HxRG_Main.mcd"),self.std_cfg))
        else:
            print("Place %s here: %s"%("HxRG_Main.mcd",self.std_cfg))
        #if isfile(join(self.hxrg_mcd,'optimisation_18859_157_diff_18dB.mcd')):
        #    system("cp %s %s"%(join(self.hxrg_mcd,"optimisation_18859_157_diff_18dB.mcd"),self.std_cfg))
        #else:
        #    print("Place %s here: %s"%("HxRG_Main.mcd",self.std_cfg))
        if isfile(join(self.hxrg_mcd,'MACIE_Registers_Slow.mrf')):
            system("cp %s %s"%(join(self.hxrg_mcd,"MACIE_Registers_Slow.mrf"),self.std_cfg))
        else:
            print("Place %s here: %s"%("MACIE_Registers_Slow.mrf",self.std_cfg))
        system("mkdir -p %s"%join(self.hxrg,'bin'))
        system("cp %s %s"%(join(self.hxrg_src,'HxRG-ENG'),join(join(self.hxrg,'bin'),TARGET)))
        system("cp %s %s"%(join(self.hxrg_src,'HxRG-ENG'),join(self.std_bin,TARGET)))    
    def uninstall(self):
        if isfile(join(self.std_cfg,'HxRG.init')):
            system("rm %s"%(join(self.std_cfg,'HxRG.init')))
        if isfile(join(self.std_cfg,'spip.conf')):
            system("rm %s"%(join(self.std_cfg,'eng.conf')))
        if isfile(join(self.std_cfg,'cmd.conf')):
            system("rm %s"%(join(self.std_cfg,'cmd.conf')))
        if isfile(join(self.std_cfg,'state.conf')):
            system("rm %s"%(join(self.std_cfg,'state.conf')))
        #if isfile(join(self.std_cfg,'optimisation_18859_157_diff_18dB.mcd') ):
        #    system("rm %s"%(join(self.std_cfg,'optimisation_18859_157_diff_18dB.mcd')))
        if isfile(join(self.std_cfg,'HxRG_Main.mcd') ):
            system("rm %s"%(join(self.std_cfg,'HxRG_Main.mcd')))
        if isfile(join(self.std_cfg,'HxRG_Main.mcd') ):
            system("rm %s"%(join(self.std_cfg,'MACIE_Registers_Slow.mrf')))
    def clean_cfg(self):
        ls = [join(self.hxrg_cfg,f) for f in listdir(self.hxrg_cfg) if '.cfg' not in f]
        for f in ls:
            system('rm %s'%f)
        
    def mod_cfg_file(self,key,value,cfgfile='HxRG.init'):
        '''
        Do not give the original config file (.cfg).

        Returns
        -------
        None.

        '''
        if '.cfg' in cfgfile:
            #we don't want to erase original .cfg file
            print("Unable to update .cfg file, only .init and .config")
            return 
           
        with open(join(self.std_cfg,cfgfile),'r') as f:
            lines = f.readlines()
        cfg = cfgentry()
        with open(join(self.std_cfg,cfgfile),'w') as f:
            for line in lines:
                if key in line:
                    cfg(line)
                    cfg.value = value
                    f.write(cfg.line())
                else:
                    f.write(line)
def help():
    print("install.py help")
    print("--clean: clean everything")
    print("--setlib: Set the MACIE, UICS and f2r dynamic library.")
    print("--config: set the configuration files in target directory.")
    print("--install: install everything in target directory.")
if '__main__' in __name__:
    from sys import argv
    from sys import exit
    from os import getenv
    if '--help' in argv or len(argv)==1:
        help()
        exit(0)
    if getenv("CONFPATH") is None:
        print("\n\n:::::::::::::::::::::::::::::::::::::::::\n")
        print("Don't forget to set CONFPATH to /opt/HxRG-SPIP/config")
        print("\n:::::::::::::::::::::::::::::::::::::::::\n")
        input("Press any key to continue")
    if '--config' in argv:
        stdIns = stdinstallation()
        stdIns.set_config()
        exit(0)
    if '--setlib' in argv:
        lib = unloadlib()
        lib.clean_lib_dir()
        lib.untar_lib()
        exit(0)
    if '--clean' in argv:
        lib = unloadlib()
        lib.clean_lib_dir()
        stdIns = stdinstallation()
        stdIns.clean_cfg()
        stdIns = stdinstallation()
        stdIns.uninstall()
        exit(0)
    if '--install' in argv:
        stdIns = stdinstallation()
        #untar MACIE library
        lib = unloadlib()
        lib.clean_lib_dir()
        lib.untar_lib()
        #Set default configuration files
        stdIns.set_config()
        #install everything else
        stdIns.install()
        print("You probably want to add %s to your bashrc file"%"/opt/HxRG-SPIP/bin")
        exit(0)
    
