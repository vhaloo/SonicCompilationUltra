# Windows virtual/on-screen keyboard messages may omit the hardware scan code.
# Resolve that case before SDL's normal scan-code translation. This is an
# idempotent, source-visible patch to the pinned SDL build only.
if(WIN32)
  set(SCU_SDL_EVENTS "${sdl2_SOURCE_DIR}/src/video/windows/SDL_windowsevents.c")
  if(NOT EXISTS "${SCU_SDL_EVENTS}")
    set(SCU_SDL_EVENTS "${CMAKE_CURRENT_SOURCE_DIR}/vendor/sdl2/src/video/windows/SDL_windowsevents.c")
  endif()
  if(EXISTS "${SCU_SDL_EVENTS}")
    file(READ "${SCU_SDL_EVENTS}" SCU_SDL_TEXT)
    if(NOT SCU_SDL_TEXT MATCHES "SCU virtual keyboard compatibility")
      set(SCU_SDL_BEFORE [=[    code = VKeytoScancode(wParam);

    if (code == SDL_SCANCODE_UNKNOWN && nScanCode <= 127) {]=])
      set(SCU_SDL_AFTER [=[    code = VKeytoScancode(wParam);

    /* SCU virtual keyboard compatibility: retain normal hardware translation. */
    if (code == SDL_SCANCODE_UNKNOWN && nScanCode == 0) {
        UINT mapped = MapVirtualKey((UINT)wParam, MAPVK_VK_TO_VSC_EX);
        nScanCode = mapped & 0xFF;
        bIsExtended = (mapped & 0xFF00) != 0;
    }

    if (code == SDL_SCANCODE_UNKNOWN && nScanCode <= 127) {]=])
      string(FIND "${SCU_SDL_TEXT}" "${SCU_SDL_BEFORE}" SCU_SDL_AT)
      if(SCU_SDL_AT LESS 0)
        message(FATAL_ERROR "Pinned SDL keyboard patch no longer matches; inspect the upstream source")
      endif()
      string(REPLACE "${SCU_SDL_BEFORE}" "${SCU_SDL_AFTER}" SCU_SDL_TEXT "${SCU_SDL_TEXT}")
      file(WRITE "${SCU_SDL_EVENTS}" "${SCU_SDL_TEXT}")
    endif()
  endif()
endif()
