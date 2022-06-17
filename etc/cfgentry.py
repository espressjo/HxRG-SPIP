#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Thu May  6 11:08:00 2021

@author: espressjo
"""



class cfgentry():
    def __init__(self,txt=''):
        
        #print(txt)
        self.key = ''
        self.value = ''
        self.cmt = ''
        if txt!='':
            self.txt = txt.replace('\t',' ')
            self._get_cmt()
            self._parser()
    def line(self):
        return "%s %s %s\n"%(self.key,self.value,self.cmt)
    def __call__(self,txt):
        self.txt = txt.replace('\t',' ')
        self._get_cmt()
        self._parser()
        return "key: %s, value: %s, cmt: %s"%(self.key,self.value,self.cmt)
    def __str__(self):
        return "key: %s, value: %s, cmt: %s"%(self.key,self.value,self.cmt)
    def _get_cmt(self):
        cmt =''
        txt = ''
        acc=False
        for c in self.txt:
            if c=='#':
                acc=True
            if acc:
                cmt+=c
            else:
                txt+=c
        self.txt = txt
        self.cmt = cmt.strip()
    def _parser(self):
        _txt = self.txt.strip().split(' ')
        #print(_txt)
        _txt = [w for w in _txt if w!='']
        if len(_txt)==1:
            self.key = _txt[0]
        elif len(_txt)==2:
            self.key,self.value = _txt
        else:
            print("[warning] Problem parsing txt.")
            self.key,self.value,self.cmt = ['','','']
        
if '__main__' in __name__:
    
    f1 = '/home/espressjo/Documents/HxRG-SERVER2/conf/HxRG.init'
    f2 = '/home/espressjo/Documents/HxRG-SERVER2/conf/nirps.conf'
    cfg = cfgentry()
    with open(f2,'r') as f:
        for line in f.readlines():
            
            print(cfg(line))