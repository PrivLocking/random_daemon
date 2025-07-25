ifeq ($(USER),root)
	$(info )
	$(info 838388381 you can NOT run by root !!! )
	$(info )
	$(error )
endif

.PHONY: all clean aaa a build

# your_secure_password_here1

inc1 := inc/yt_common.h

source3 := src/random_yt_daemon4.c
source4 := src/channel_slot_assign.c
source5 := src/html_logic.c

sourceListIdx:=3 4 5
sourceList:=$(foreach ssss,$(sourceListIdx),$(source$(ssss)))



dst     := bin/yt_random_daemon.bin
CFLAGS := -Wall -O3 -static
CFLAGS := -Wall -Os -static
CFLAGS := -Wall -Os -ffunction-sections -fdata-sections -Wl,--gc-sections -static -D_FORTIFY_SOURCE=0 -DNDEBUG
CFLAGS := -Wall -Oz -ffunction-sections -fdata-sections -Wl,--gc-sections -static -D_FORTIFY_SOURCE=0 -DNDEBUG
CFLAGS := -Wall -O3 -ffunction-sections -fdata-sections -Wl,--gc-sections -static
CFLAGS := -Wall -O3 -ffunction-sections -fdata-sections -Wl,--gc-sections -static
CFLAGS := -Wall -O3 -ffunction-sections -fdata-sections -Wl,--gc-sections
CFLAGS := -Wall -O3 -ffunction-sections -fdata-sections -Wl,--gc-sections -I.
CFLAGS := -Wall -O3 -ffunction-sections -fdata-sections -Wl,--gc-sections -I. -mno-avx -mno-avx2
CFLAGS := -Wall -O2 -ffunction-sections -fdata-sections -Wl,--gc-sections -I. -mno-avx -mno-avx2

LDLIBS := -lgd -lpng -lz -ljpeg -lfreetype -lm -lmicrohttpd -lgnutls
LDLIBS := -lgd -lpng -lz -ljpeg -lfreetype -lm
LDLIBS := -lgd -lpng -lz -ljpeg -lfreetype -lm -lfontconfig -lbrotlidec -lbrotlicommon -lbz2 -lexpat
LDLIBS := -lgd -lpng -lz -ljpeg -lfreetype -lm -lfontconfig -lbrotlidec -lbrotlicommon -lbz2 -lexpat  -lssl -lcrypto
LDLIBS := -lhiredis -ljansson -largon2 -lssl -lcrypto
#-lbrotlicommon -lbrotlienc
installDir:=/home/nginX/bin/
installBin1:=/home/nginX/bin/$(shell basename $(dst))


all:
	@echo "$${allTEXT}"

define allTEXT

aaa : $(aaa)
sourceList:=$(sourceList)
vv: vim file list
t test run_test


endef
export allTEXT

b:ball

v:v3

vi: vi1
vi1 : $(inc1) vp
	vim $<

m:
	vim Makefile

#v1:
#	vim $(source1)
define vimFun
v$(1): vp
	vim $(source$(1))
endef
$(foreach vimvim,$(sourceListIdx),$(eval $(call vimFun,$(vimvim))))
vv:
	@echo
	@$(foreach ssss,$(sourceListIdx), echo "$(ssss) : $(source$(ssss))"; )
	@echo

aaa := make c && make ball && make in
aaa :
	make sign
	$(aaa)


b1 : $(mid1)
#	strip --strip-all --strip-unneeded $<
#	md5sum $<
b2 : $(mid2)
#	strip --strip-all --strip-unneeded $<
#	md5sum $<
b3 : $(mid3)
#	strip --strip-all --strip-unneeded $<
#	md5sum $<
b4 : $(mid4)
#	strip --strip-all --strip-unneeded $<
#	md5sum $<

c clean:
	rm -rf bin/*.bin tmp/*.o


i in install:
	@echo
	-chmod   u+w   $(installDir)/
	cat   $(dst)   > $(installBin1)
	@ls -l --color   $(installBin1)
	@md5sum          $(installBin1)
	@chmod 755       $(installBin1)
	@echo

vpc:
	rm -f tags \
		cscope.in.out \
		cscope.out \
		cscope.po.out
	rm -f _vim/file01.txt

vp vim_prepare : vpc
	mkdir -p _vim/
	echo Makefile                                        > _vim/file01.txt
	-test -f Makefile.env && echo Makefile.env          >> _vim/file01.txt
	find -type f -name "*.c" -o -name "*.h" \
		|grep -v '\.bak[0-9]*' \
		| xargs -n 1 realpath --relative-to=.|sort -u   >> _vim/file01.txt
	sed -i -e '/^\.$$/d' -e '/^$$/d'                       _vim/file01.txt
	cscope -q -R -b -i                                     _vim/file01.txt
	ctags -L                                               _vim/file01.txt
gs:
	git status
gd:
	git diff
gc:
	git add .
	git commit -a

#	git_ssh_example.sh git push
up:
	git push
	sync


#	$(CC) $(CFLAGS) -static -o $(dst) \
#	$(CC) $(CFLAGS) -c -o $(mid1) $(source1) $(LDLIBS)

ball:
	@mkdir -p bin/
	$(CC) $(CFLAGS) -static -o $(dst) \
		$(sourceList)   \
		$(LDLIBS)
	strip $(dst)
	md5sum `realpath $(dst)` /home/nginX/bin/yt_random_daemon.bin

tSock:=test/sock.test.sock
tSock2:=/lxc_/u99/wwwFS.out/sock.youtube.recommend.sock
tSrc:=src/videos.newest.120.txt
t1 test run_test_daemon:
	@echo
	@mkdir -p test/
	${dst}   -s $(tSock) -f $(tSrc)
	@echo
t2 run_curl_test:
	@echo
	curl --unix-socket $(tSock)   http://ip.jjj123.com/ -v 
	@echo
	curl --unix-socket $(tSock)   -s http://ip.jjj123.com/ |grep window.location.href
	curl --unix-socket $(tSock)   -s http://ip.jjj123.com/abcd |grep window.location.href
	curl --unix-socket $(tSock)   -s http://ip.jjj123.com/abcd/ |grep window.location.href
	curl --unix-socket $(tSock)   -s http://ip.jjj123.com/abcd/efg |grep window.location.href
	curl --unix-socket $(tSock)   -s http://ip.jjj123.com/abcd/efg/ |grep window.location.href
	curl --unix-socket $(tSock)   -s http://ip.jjj123.com/abcd/efg/efg |grep window.location.href
	curl --unix-socket $(tSock2)   -s http://ip.jjj123.com/abcd/efg/efg |grep window.location.href
	@echo
t3 run_test_server:
	@echo
	curl -s --resolve "www.jjj123.com:443:74.131.213.118" https://www.jjj123.com/recommend/aa |grep href
	@echo
	curl -s --resolve "www.jjj123.com:443:74.131.213.118" https://www.jjj123.com/recommend/   |grep href
	@echo
t4 run_test_server_on_u99:
	@echo
	curl --unix-socket $(tSock2)   -s http://www.jjj123.com/abcd/efg/efg |grep window.location.href
	@echo

define t9Help

curl --unix-socket /home/nginY/dynProg/301_youtube_random_daemon/tmp/sock.youtube.recommend.sock  http://www.jjj123.com/recommend/ -s |grep window.location.href
curl --unix-socket /home/nginY/dynProg/301_youtube_random_daemon/tmp/sock.youtube.recommend.sock  http://www.jjj123.com/recommend/aa -s |grep window.location.href

endef
export t9Help
t9FF:=$(firstword $(wildcard ~/u12/22/???))
t9F1:=$(t9FF)/dst01.txt
t9F2:=$(t9FF)/dst02.txt
t9FS:=$(shell realpath tmp )/sock.youtube.recommend.sock
t9 run_as_daemon:
	@echo
	@echo "$${t9Help}"
	$(dst) -f $(t9F1) -2 $(t9F2) -s $(t9FS)
	@echo

