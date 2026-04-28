# NMAKE Makefile for eikana-alt
#
# Run from "Developer Command Prompt for VS":
#   nmake           build (incremental)
#   nmake clean     remove build outputs

OUTDIR = build
EXE    = $(OUTDIR)\eikana-alt.exe

CFLAGS = /nologo /MP /W4 /EHsc /O2 /std:c++20 /utf-8 \
         /DUNICODE /D_UNICODE /Isrc /permissive-
LFLAGS = /SUBSYSTEM:WINDOWS user32.lib shell32.lib advapi32.lib

OBJS = $(OUTDIR)\main.obj \
       $(OUTDIR)\AltSoloDetector.obj \
       $(OUTDIR)\AppState.obj \
       $(OUTDIR)\Autostart.obj \
       $(OUTDIR)\CapsRemapInstaller.obj \
       $(OUTDIR)\ImeController.obj \
       $(OUTDIR)\KeyboardHook.obj \
       $(OUTDIR)\TrayIcon.obj

HEADERS = src\AltSoloDetector.h src\AppState.h src\Autostart.h \
          src\CapsRemapInstaller.h src\ImeController.h src\InjectedTag.h \
          src\KeyboardHook.h src\ModulePath.h src\Resource.h src\TrayIcon.h

all: $(EXE)

$(EXE): $(OBJS)
	@if not exist $(OUTDIR) mkdir $(OUTDIR)
	link /NOLOGO /OUT:$@ $(LFLAGS) $(OBJS)

# Coarse-grained: any header change rebuilds all objects.
$(OBJS): $(HEADERS)

# Batch-mode inference rule: NMAKE collects all out-of-date .cpp files
# under src\ and feeds them to a single cl invocation, which then
# parallelises across them via /MP.
{src\}.cpp{$(OUTDIR)\}.obj::
	@if not exist $(OUTDIR) mkdir $(OUTDIR)
	cl $(CFLAGS) /c /Fo$(OUTDIR)\ /Fd$(OUTDIR)\ $<

clean:
	@if exist $(OUTDIR) rmdir /s /q $(OUTDIR)
