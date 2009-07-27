The default JDK on Mac is 1.5
This is set by the symbolic link:

/System/Library/Frameworks/JavaVM.framework/Versions/Current

Which is normally set to point to "A"

To change your java version globally set it to 1.6



If you want to change it in a launch script add the following:

# This is needed for the "java" command line
JAVA_HOME=/System/Library/Frameworks/JavaVM.framework/Versions/1.6.0/Home
export JAVA_HOME

# This is needed for the native launcher
JAVA_JVM_VERSION=1.6
export JAVA_JVM_VERSION
