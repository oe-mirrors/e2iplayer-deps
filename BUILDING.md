# Building

Each component has a `Makefile` (or `makefile` for `hlsdl`). They honour the
usual `CC`, `CFLAGS`, `CPPFLAGS`, `LDFLAGS`, `STRIP` variables, so the same
Makefiles work for a native build and for an OpenEmbedded cross build.

```sh
# native (what CI does)
make -C hlsdl
make -C cmdwrap
make -C lsdir
make -C e2isubparser            # needs python3 dev headers

# cross (OE)
make -C cmdwrap CC="${CC}" CFLAGS="${CFLAGS}" LDFLAGS="${LDFLAGS}"
make -C e2isubparser CC="${CC}" CFLAGS="${CFLAGS}" LDFLAGS="${LDFLAGS}" \
     PYTHON_INCLUDES="-I${STAGING_INCDIR}/python3.x"
```

Outputs: `hlsdl/hlsdl`, `cmdwrap/cmdwrap`, `lsdir/lsdir`,
`e2isubparser/_subparser.so`.

The old `make.sh` / `doall.sh` scripts were hard-coded to one developer's
machine and Python 2.7 and have been removed. `f4mdump` (Adobe HDS / RTMP)
still has no build file - it is Flash-era and unused in practice.

CI (`.github/workflows/ci.yml`) builds every component on each push.
