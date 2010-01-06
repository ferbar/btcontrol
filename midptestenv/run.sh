#!/bin/sh
# java -cp ./javapng-2.0.jar:./me4se-3.1.7.jar:./btcontroll.jar:./avetanaBT.jar org.me4se.MIDletRunner btcontroll.HelloMidlet
java -cp ./me4se-3.1.7.jar:./btcontroll.jar:./avetanaBT.jar \
	org.me4se.MIDletRunner \
	-screen.width 500 \
	-screen.height 500
