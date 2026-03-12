obj-m += hello.o
hello-objs := src/hello.o

obj-m += monitor.o
monitor-objs := src/main.o

obj-m += schedinfo.o
schedinfo-objs := src/schedinfo.o

# For multiple modules in subdirectories:
# obj-m += module2.o
# module2-objs := src/module2/main.o src/module2/helper.o

ccflags-y := -I$(src)/src
