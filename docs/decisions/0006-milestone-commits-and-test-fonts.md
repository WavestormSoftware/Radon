# 0006: Milestone commits and test fonts

- The core was written in one pass, so milestone commits M1, M2, M4 and M5 group files by content. Only
  the tree from M5 onward builds as a whole (the CMake file lists all core sources); M0-M4 were not built
  or tested as separate commits.
- No font ships with the repo (the owner supplies `examples/assets/font.ttf`). Font-dependent tests look at
  `$RD_TEST_FONT`, then `examples/assets/font.ttf`, then common system fonts, and print a skip message when
  none is found. The local run used the system Noto Sans; nothing from it is committed.
