#!/bin/bash

svn log --xml --verbose | xsltproc scripts/svn2cl.xsl - > ChangeLog
