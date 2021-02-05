cmake_minimum_required(VERSION 3.5)

set(manpages
  pekwm.1
  pekwm_bg.1
  pekwm_dialog.1
  pekwm_theme.1)

if (ENABLE_IMAGE_PNG AND PNG_FOUND)
  set(manpages ${manpages} pekwm_screenshot.1)
endif (ENABLE_IMAGE_PNG AND PNG_FOUND)

install(FILES
  ${manpages}
  DESTINATION share/man/man1)
