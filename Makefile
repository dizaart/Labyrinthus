CC = g++
CPPFLAGS=-g -Isrc/sockets/ -Isrc/objects -Isrc/common -Isrc/utils -I/Applications/Xcode.app/Contents//Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.8.sdk/usr/include/c++/4.2.1/ -I /Applications/Xcode.app/Contents//Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.8.sdk/usr/include/ -ferror-limit=1
LDFLAGS=-pthread
main: svnco svnci clean RSServer RSRouter RSDNSServer RSServerCommunication RSTunnelServer 
	#strip --remove-section .comment --remove-section .note OnDeviceConnect rs_server	
RSServer: src/apps/RSServer.o src/sockets/udpsocket.o src/objects/rssobject.o src/common/rs_defines.o src/utils/get_crc.o
RSRouter: src/apps/RSRouter.o src/sockets/udpsocket.o src/objects/rssobject.o
RSDNSServer: src/apps/RSDNSServer.o src/sockets/udpsocket.o src/objects/rssobject.o
RSServerCommunication: src/apps/RSServerCommunication.o src/sockets/udpsocket.o src/objects/rssobject.o
RSTunnelServer: src/apps/RSTunnelServer.o src/sockets/udpsocket.o src/objects/rssobject.o
svnco:
	svn up
svnci:
	svn ci -m ''
clean:
	rm -rf src/apps/*.o
	rm -rf src/utils/*.o
	rm -rf src/objects/*.o
	rm -rf src/sockets/*.o
	rm -rf src/common/*.o
	rm -f src/apps/RSServer
	rm -f src/apps/RSRouter
	rm -f src/apps/RSDNSServer
	rm -f src/apps/RSServerCommunication
	rm -f src/apps/RSTunnelServer
