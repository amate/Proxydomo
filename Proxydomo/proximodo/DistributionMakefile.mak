# Project: Proximodo
# Makefile for inclusion in main makefile, that copies required files into distribution package

all-after: ../bin/Proximodo.exe
	xcopy distrib ..\bin /I /Y /E /Q
