#Just a stub, jumpstarts the cmake Makefile generation and the build process

BUILD_DIR := build

.PHONY: all doc clean mrproper distclean install uninstall help format

all:
	@cd ${BUILD_DIR} && ./.build $(args)
doc:
	@cd ${BUILD_DIR} && ./.build -t doc $(args)
clean:
	@cd ${BUILD_DIR} && ./.clean $(args)
mrproper:
	@cd ${BUILD_DIR} && ./.clean -g $(args)
distclean:
	@cd ${BUILD_DIR} && ./.clean -d -g -e $(args)
dev-format:
	@cd scripts && ./.format

install:
	@cd ${BUILD_DIR} && make -s install

uninstall:
	@cd ${BUILD_DIR} && make -s uninstall

help:
	@printf "Usage:\n\n  make [args=\"opt1 opt2 ...\"] [<stub_target>]\n\nThis is a stub build file that can be used to trigger compiling rscfl\ncomponents and their dependencies in a standard way. The actual build\nprocess relies on the cmake build system, which is bootstrapped if it\ncan't be found in your PATH.\n\n"
	@printf "  <stub_target> can be one of:"
	@$(MAKE) -pRrq -f $(lastword $(MAKEFILE_LIST)) : 2>/dev/null | awk -v RS= -F: '/^# File/,/^# Finished Make data base/ {if ($$1 !~ "^[#.]|Makefile") {printf "    "; print $$1;}}'
	@printf "\n stub_target = all (default):\n\n  The following options can be passed to the underlying build script:\n  opt1, opt2, ... :\n"
	@cd ${BUILD_DIR} && ./.build -h | tail -n +3 | awk '{print "   " $$0}'
	@printf "\n    <TARGETS> available for the -t option: \n    Those are passed as targets to the actual build script:\n\n"
	@cd ${BUILD_DIR} && ./.build -xs0 >/dev/null 2>&1 && make help | tail -n +3 | sed 's/.../   /' | sed \$$d
	@printf "\n stub_target = clean: \n\n"
	@printf "  The following options are available opt1, opt2, ...:\n"
	@cd ${BUILD_DIR} && ./.clean -h | tail -n +3 | awk '{print "   " $$0}'
	@printf "\nPlease read more about building rscfl from source from README.md\n"

%:
	@:
