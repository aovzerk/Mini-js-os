require("myos")

myos.write("MyOS JavaScript API test\n")
myos.writeFile("APIOUT.TXT", "Written from JavaScript\n")

myos.write("Reading APIOUT.TXT:\n")
myos.readFile("APIOUT.TXT")

myos.write("Root directory:\n")
myos.listFiles()

myos.getPid()
myos.getAppType()
myos.yield()

// JS.BIN is a console application, so kernel policy returns -1 here.
myos.createWindow()

myos.write("API test complete\n")
