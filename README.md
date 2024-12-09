# GtkBaseApplication

This is an application that can serve as a base for projects,
that use gtk4 and libadwaita.

## How to build?

`meson setup --wipe builddir`\
`meson compile -C builddir`\
\
The gschema file needs to be installed for the program to run.\
On my system it installs to `/usr/local/`\
\
`meson install -C builddir`\
\
search for Test App or run:
`./builddir/src/test_app`
