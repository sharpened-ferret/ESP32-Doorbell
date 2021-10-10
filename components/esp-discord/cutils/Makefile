all:
MAKEFLAGS  += -r

INCDIR      = include
SRCDIR      = src
BUILDDIR    = build
OBJDIR      = ${BUILDDIR}/obj
BINDIR      = ${BUILDDIR}/bin
TESTSSRC    = tests
TESTSBIN    = ${BINDIR}/tests

CCFLAGS     = -I${INCDIR} -MD -MP
CCWARNINGS  = -Wall -Wextra# -Wpedantic
CC          = gcc ${CCFLAGS} ${CCWARNINGS}

ifeq ($(OS),Windows_NT)
    MKDIR = mkdir
	TOUCH = echo >
	CHECKPASS = if ERRORLEVEL 0 echo. & echo Test "X" passed & echo.
	RMDIR = rd /s /q
else
    MKDIR = mkdir -p
	TOUCH = touch
	CHECKPASS = test $$$$? -eq 0 && echo "\nTest \"X\" passed\n"
	RMDIR = rm -rf
endif

.PRECIOUS: %/.sentinel
%/.sentinel:
	${MKDIR} "${@D}"
	@${TOUCH} "$@"

clean:
	${RMDIR} "./$(BUILDDIR)"
	@echo "Project cleaned"

COMPONENTS =
define add_component # {1} - name; {2} - sources
${1}: $(patsubst %.c,${OBJDIR}/%.o,${2})
COMPONENTS += ${1}
COMPONENT_${1}_OBJS = $(patsubst %.c,${OBJDIR}/%.o,${2})
-include $(patsubst %.c,${OBJDIR}/%.d,${2})
endef

${OBJDIR}/%.o: ${SRCDIR}/%.c ${OBJDIR}/.sentinel Makefile
	${CC} -c -o $@ $<

# COMPONENTS

$(eval $(call add_component,estr,estr.c))
$(eval $(call add_component,cutils))
$(eval $(call add_component,xlist,xlist.c))
$(eval $(call add_component,wxp,estr.c wxp.c))
$(eval $(call add_component,cmder,estr.c xlist.c wxp.c cmder.c))

all: ${COMPONENTS}

COMPONENTS_TESTS =
define add_component_test # {1} - component name; {2} - extra dependent sources
test.${1}: ${1} $(patsubst %.c,${OBJDIR}/%.o,${2}) ${TESTSBIN}/${1}.test
	./${TESTSBIN}/${1}.test
	@$(subst X,${1},${CHECKPASS})
COMPONENTS_TESTS += test.${1}
COMPONENTS_TESTS_${1}_OBJS = $(patsubst %.c,${OBJDIR}/%.o,${2})
${TESTSBIN}/${1}.test: ${COMPONENT_${1}_OBJS} $(patsubst %.c,${OBJDIR}/%.o,${2})
-include ${TESTSBIN}/${1}.d
endef

${TESTSBIN}/%.test: ${TESTSSRC}/%.test.c ${TESTSBIN}/.sentinel Makefile
	${CC} -o $@ $< ${COMPONENT_${*}_OBJS} ${COMPONENTS_TESTS_${*}_OBJS}

# TESTS

$(eval $(call add_component_test,cutils,estr.c))
$(eval $(call add_component_test,estr))
$(eval $(call add_component_test,xlist))
$(eval $(call add_component_test,wxp))
$(eval $(call add_component_test,cmder))

test: ${COMPONENTS_TESTS}