include Makefile.include

AM_CPPFLAGS += -I$(builddir)

noinst_LIBRARIES = \
  libbitmark_server.a \
  libbitmark_common.a \
  libbitmark_cli.a
if ENABLE_WALLET
noinst_LIBRARIES += libbitmark_wallet.a
endif

bin_PROGRAMS =

if BUILD_BITMARKD
  bin_PROGRAMS += bitmarkd
endif

if BUILD_BITMARK_CLI
  bin_PROGRAMS += bitmark-cli
endif

SUBDIRS = . $(BUILD_QT) $(BUILD_TEST)
DIST_SUBDIRS = . qt test
.PHONY: FORCE
# bitmark core #
BITMARK_CORE_H = \
  addrman.h \
  alert.h \
  allocators.h \
  base58.h bignum.h \
  bloom.h \
  chainparams.h \
  checkpoints.h \
  checkqueue.h \
  clientversion.h \
  coincontrol.h \
  coins.h \
  compat.h \
  core.h \
  crypter.h \
  db.h \
  hash.h \
  init.h \
  key.h \
  keystore.h \
  leveldbwrapper.h \
  limitedmap.h \
  main.h \
  miner.h \
  mruset.h \
  netbase.h \
  net.h \
  noui.h \
  pow.h \
  protocol.h \
  pureheader.h \
  rpcclient.h \
  rpcprotocol.h \
  rpcserver.h \
  script.h \
  scrypt.h \
  serialize.h \
  sync.h \
  threadsafety.h \
  tinyformat.h \
  txdb.h \
  txmempool.h \
  ui_interface.h \
  uint256.h \
  util.h \
  version.h \
  walletdb.h \
  wallet.h

JSON_H = \
  json/json_spirit.h \
  json/json_spirit_error_position.h \
  json/json_spirit_reader.h \
  json/json_spirit_reader_template.h \
  json/json_spirit_stream_reader.h \
  json/json_spirit_utils.h \
  json/json_spirit_value.h \
  json/json_spirit_writer.h \
  json/json_spirit_writer_template.h

build.h: FORCE
	@$(MKDIR_P) $(abs_top_builddir)/src/obj
	@$(top_srcdir)/share/genbuild.sh $(abs_top_builddir)/src/build.h \
	  $(abs_top_srcdir)
version.o: build.h

libbitmark_server_a_SOURCES = \
  addrman.cpp \
  alert.cpp \
  bloom.cpp \
  checkpoints.cpp \
  coins.cpp \
  init.cpp \
  keystore.cpp \
  leveldbwrapper.cpp \
  main.cpp \
  miner.cpp \
  net.cpp \
  noui.cpp \
  rpcblockchain.cpp \
  rpcmining.cpp \
  rpcmisc.cpp \
  rpcnet.cpp \
  rpcrawtransaction.cpp \
  rpcserver.cpp \
  txdb.cpp \
  txmempool.cpp \
  $(JSON_H) \
  $(BITMARK_CORE_H)

libbitmark_wallet_a_SOURCES = \
  db.cpp \
  crypter.cpp \
  rpcdump.cpp \
  rpcwallet.cpp \
  wallet.cpp \
  walletdb.cpp \
  $(BITMARK_CORE_H)

libbitmark_common_a_SOURCES = \
  base58.cpp \
  allocators.cpp \
  chainparams.cpp \
  core.cpp \
  hash.cpp \
  key.cpp \
  netbase.cpp \
  pow.cpp \
  pureheader.cpp \
  protocol.cpp \
  rpcprotocol.cpp \
  script.cpp \
  scrypt.cpp \
  sync.cpp \
  util.cpp \
  version.cpp \
  groestl.c \
  blake.c \
  blake2s-ref.c \
  bmw.c \
  cubehash.c \
  echo.c \
  jh.c \
  keccak.c \
  luffa.c \
  shavite.c \
  simd.c \
  skein.c \
  hamsi.c \
  whirlpool.c \
  shabal.c \
  fugue.c \
  sync.cpp \
  sha2.c \
  haval.c \
  Lyra2.c \
  Lyra2RE.c \
  Sponge.c \
  sph_sha2big.c \
  ar2/argon2.c \
  ar2/core.c \
  ar2/encoding.c \
  ar2/genkat.c \
  ar2/opt.c \
  ar2/ref.c \
  ar2/run.c \
  ar2/test.c \
  ar2/thread.c \
  ar2/blake2/blake2b.c \
  equihash.cpp \
  equihash.tcc \
  cryptonight/crypto/slow-hash.c \
  cryptonight/crypto/oaes_lib.c \
  cryptonight/common/c_keccak.c \
  cryptonight/crypto/aesb.c \
  cryptonight/crypto/hash.c \
  cryptonight/crypto/hash-extra-blake.c \
  cryptonight/crypto/hash-extra-groestl.c \
  cryptonight/crypto/hash-extra-jh.c \
  cryptonight/crypto/hash-extra-skein.c \
  cryptonight/crypto/blake256.c \
  cryptonight/crypto/groestl.c \
  cryptonight/crypto/jh.c \
  cryptonight/crypto/skein.c \
  yescrypt/yescryptcommon.c \
  yescrypt/yescrypt-best.c \
  yescrypt/yescrypt-platform.c \
  yescrypt/sha256_Y.c \
  $(BITMARK_CORE_H)

if GLIBC_BACK_COMPAT
libbitmark_common_a_SOURCES += compat/glibc_compat.cpp
libbitmark_common_a_SOURCES += compat/glibcxx_compat.cpp
endif

libbitmark_cli_a_SOURCES = \
  rpcclient.cpp \
  $(BITMARK_CORE_H)

nodist_libbitmark_common_a_SOURCES = $(top_srcdir)/src/build.h
#

# bitmarkd binary #
bitmarkd_LDADD = \
  libbitmark_server.a \
  libbitmark_cli.a \
  libbitmark_common.a \
  $(LIBLEVELDB) \
  $(LIBMEMENV)

if ENABLE_WALLET
bitmarkd_LDADD += libbitmark_wallet.a
endif
bitmarkd_SOURCES = bitmarkd.cpp
#

if TARGET_WINDOWS
bitmarkd_SOURCES += bitmarkd-res.rc
endif

AM_CPPFLAGS += $(BDB_CPPFLAGS)
bitmarkd_LDADD += $(BOOST_LIBS) $(BDB_LIBS)

# bitmark-cli binary #
bitmark_cli_LDADD = \
  libbitmark_cli.a \
  libbitmark_common.a \
  $(BOOST_LIBS)
bitmark_cli_SOURCES = bitmark-cli.cpp
#

if TARGET_WINDOWS
bitmark_cli_SOURCES += bitmark-cli-res.rc
endif

# NOTE: This dependency is not strictly necessary, but without it make may try to build both in parallel, which breaks the LevelDB build system in a race
leveldb/libleveldb.a: leveldb/libmemenv.a

leveldb/%.a:
	@echo "Building LevelDB ..." && $(MAKE) -C $(@D) $(@F) CXX="$(CXX)" \
	  CC="$(CC)" PLATFORM=$(TARGET_OS) AR="$(AR)" $(LEVELDB_TARGET_FLAGS) \
	  OPT="$(CXXFLAGS) $(CPPFLAGS)"

qt/bitmarkstrings.cpp: $(libbitmark_server_a_SOURCES) $(libbitmark_common_a_SOURCES) $(libbitmark_cli_a_SOURCES)
	@test -n $(XGETTEXT) || echo "xgettext is required for updating translations"
	@cd $(top_srcdir); XGETTEXT=$(XGETTEXT) share/qt/extract_strings_qt.py

CLEANFILES = leveldb/libleveldb.a leveldb/libmemenv.a *.gcda *.gcno

DISTCLEANFILES = build.h

EXTRA_DIST = leveldb Makefile.include

clean-local:
	-$(MAKE) -C leveldb clean
	rm -f leveldb/*/*.gcno leveldb/helpers/memenv/*.gcno
