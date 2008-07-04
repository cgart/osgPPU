#!/bin/bash

svn up ../
svn log --xml --verbose ../ | xsltproc scripts/svn2cl.xsl - > ../ChangeLog
