include $(top_srcdir)/src/Makefile.include

AM_CPPFLAGS += -I$(top_srcdir)/src \
  -I$(top_builddir)/src/qt \
  -I$(top_builddir)/src/qt/forms \
  $(PROTOBUF_CFLAGS) \
  $(QR_CFLAGS)
bin_PROGRAMS = bitmark-qt
noinst_LIBRARIES = libbitmarkqt.a
SUBDIRS = . $(BUILD_TEST_QT)
DIST_SUBDIRS = . test

# bitmark qt core #
QT_TS = \
  locale/bitmark_ach.ts \
  locale/bitmark_af_ZA.ts \
  locale/bitmark_ar.ts \
  locale/bitmark_be_BY.ts \
  locale/bitmark_bg.ts \
  locale/bitmark_bs.ts \
  locale/bitmark_ca_ES.ts \
  locale/bitmark_ca.ts \
  locale/bitmark_ca@valencia.ts \
  locale/bitmark_cmn.ts \
  locale/bitmark_cs.ts \
  locale/bitmark_cy.ts \
  locale/bitmark_da.ts \
  locale/bitmark_de.ts \
  locale/bitmark_el_GR.ts \
  locale/bitmark_en.ts \
  locale/bitmark_eo.ts \
  locale/bitmark_es_CL.ts \
  locale/bitmark_es_DO.ts \
  locale/bitmark_es_MX.ts \
  locale/bitmark_es.ts \
  locale/bitmark_es_UY.ts \
  locale/bitmark_et.ts \
  locale/bitmark_eu_ES.ts \
  locale/bitmark_fa_IR.ts \
  locale/bitmark_fa.ts \
  locale/bitmark_fi.ts \
  locale/bitmark_fr_CA.ts \
  locale/bitmark_fr.ts \
  locale/bitmark_gl.ts \
  locale/bitmark_gu_IN.ts \
  locale/bitmark_he.ts \
  locale/bitmark_hi_IN.ts \
  locale/bitmark_hr.ts \
  locale/bitmark_hu.ts \
  locale/bitmark_id_ID.ts \
  locale/bitmark_it.ts \
  locale/bitmark_ja.ts \
  locale/bitmark_ka.ts \
  locale/bitmark_kk_KZ.ts \
  locale/bitmark_ko_KR.ts \
  locale/bitmark_ky.ts \
  locale/bitmark_la.ts \
  locale/bitmark_lt.ts \
  locale/bitmark_lv_LV.ts \
  locale/bitmark_mn.ts \
  locale/bitmark_ms_MY.ts \
  locale/bitmark_nb.ts \
  locale/bitmark_nl.ts \
  locale/bitmark_pam.ts \
  locale/bitmark_pl.ts \
  locale/bitmark_pt_BR.ts \
  locale/bitmark_pt_PT.ts \
  locale/bitmark_ro_RO.ts \
  locale/bitmark_ru.ts \
  locale/bitmark_sah.ts \
  locale/bitmark_sk.ts \
  locale/bitmark_sl_SI.ts \
  locale/bitmark_sq.ts \
  locale/bitmark_sr.ts \
  locale/bitmark_sv.ts \
  locale/bitmark_th_TH.ts \
  locale/bitmark_tr.ts \
  locale/bitmark_uk.ts \
  locale/bitmark_ur_PK.ts \
  locale/bitmark_uz@Cyrl.ts \
  locale/bitmark_vi.ts \
  locale/bitmark_vi_VN.ts \
  locale/bitmark_zh_CN.ts \
  locale/bitmark_zh_HK.ts \
  locale/bitmark_zh_TW.ts

QT_FORMS_UI = \
  forms/aboutdialog.ui \
  forms/addressbookpage.ui \
  forms/askpassphrasedialog.ui \
  forms/coincontroldialog.ui \
  forms/editaddressdialog.ui \
  forms/helpmessagedialog.ui \
  forms/intro.ui \
  forms/openuridialog.ui \
  forms/optionsdialog.ui \
  forms/overviewpage.ui \
  forms/receivecoinsdialog.ui \
  forms/receiverequestdialog.ui \
  forms/rpcconsole.ui \
  forms/sendcoinsdialog.ui \
  forms/sendcoinsentry.ui \
  forms/signverifymessagedialog.ui \
  forms/transactiondescdialog.ui

QT_MOC_CPP = \
  moc_addressbookpage.cpp \
  moc_addresstablemodel.cpp \
  moc_askpassphrasedialog.cpp \
  moc_bitmarkaddressvalidator.cpp \
  moc_bitmarkamountfield.cpp \
  moc_bitmarkgui.cpp \
  moc_bitmarkunits.cpp \
  moc_clientmodel.cpp \
  moc_coincontroldialog.cpp \
  moc_coincontroltreewidget.cpp \
  moc_csvmodelwriter.cpp \
  moc_editaddressdialog.cpp \
  moc_guiutil.cpp \
  moc_intro.cpp \
  moc_macdockiconhandler.cpp \
  moc_macnotificationhandler.cpp \
  moc_monitoreddatamapper.cpp \
  moc_notificator.cpp \
  moc_openuridialog.cpp \
  moc_optionsdialog.cpp \
  moc_optionsmodel.cpp \
  moc_overviewpage.cpp \
  moc_paymentserver.cpp \
  moc_qvalidatedlineedit.cpp \
  moc_qvaluecombobox.cpp \
  moc_receivecoinsdialog.cpp \
  moc_receiverequestdialog.cpp \
  moc_recentrequeststablemodel.cpp \
  moc_rpcconsole.cpp \
  moc_sendcoinsdialog.cpp \
  moc_sendcoinsentry.cpp \
  moc_signverifymessagedialog.cpp \
  moc_splashscreen.cpp \
  moc_trafficgraphwidget.cpp \
  moc_transactiondesc.cpp \
  moc_transactiondescdialog.cpp \
  moc_transactionfilterproxy.cpp \
  moc_transactiontablemodel.cpp \
  moc_transactionview.cpp \
  moc_utilitydialog.cpp \
  moc_walletframe.cpp \
  moc_walletmodel.cpp \
  moc_walletview.cpp

BITMARK_MM = \
  macdockiconhandler.mm \
  macnotificationhandler.mm

QT_MOC = \
  bitmark.moc \
  intro.moc \
  overviewpage.moc \
  rpcconsole.moc

QT_QRC_CPP = qrc_bitmark.cpp
QT_QRC = bitmark.qrc

PROTOBUF_CC = paymentrequest.pb.cc
PROTOBUF_H = paymentrequest.pb.h
PROTOBUF_PROTO = paymentrequest.proto

BITMARK_QT_H = \
  addressbookpage.h \
  addresstablemodel.h \
  askpassphrasedialog.h \
  bitmarkaddressvalidator.h \
  bitmarkamountfield.h \
  bitmarkgui.h \
  bitmarkunits.h \
  clientmodel.h \
  coincontroldialog.h \
  coincontroltreewidget.h \
  csvmodelwriter.h \
  editaddressdialog.h \
  guiconstants.h \
  guiutil.h \
  intro.h \
  macdockiconhandler.h \
  macnotificationhandler.h \
  monitoreddatamapper.h \
  notificator.h \
  openuridialog.h \
  optionsdialog.h \
  optionsmodel.h \
  overviewpage.h \
  paymentrequestplus.h \
  paymentserver.h \
  qvalidatedlineedit.h \
  qvaluecombobox.h \
  receivecoinsdialog.h \
  receiverequestdialog.h \
  recentrequeststablemodel.h \
  rpcconsole.h \
  sendcoinsdialog.h \
  sendcoinsentry.h \
  signverifymessagedialog.h \
  splashscreen.h \
  trafficgraphwidget.h \
  transactiondesc.h \
  transactiondescdialog.h \
  transactionfilterproxy.h \
  transactionrecord.h \
  transactiontablemodel.h \
  transactionview.h \
  utilitydialog.h \
  walletframe.h \
  walletmodel.h \
  walletmodeltransaction.h \
  walletview.h \
  winshutdownmonitor.h

RES_ICONS = \
  res/icons/add.png \
  res/icons/address-book.png \
  res/icons/bitmark.ico \
  res/icons/bitmark.png \
  res/icons/bitmark_testnet.ico \
  res/icons/bitmark_testnet.png \
  res/icons/clock1.png \
  res/icons/clock2.png \
  res/icons/clock3.png \
  res/icons/clock4.png \
  res/icons/clock5.png \
  res/icons/configure.png \
  res/icons/connect0_16.png \
  res/icons/connect1_16.png \
  res/icons/connect2_16.png \
  res/icons/connect3_16.png \
  res/icons/connect4_16.png \
  res/icons/debugwindow.png \
  res/icons/edit.png \
  res/icons/editcopy.png \
  res/icons/editpaste.png \
  res/icons/export.png \
  res/icons/filesave.png \
  res/icons/history.png \
  res/icons/key.png \
  res/icons/lock_closed.png \
  res/icons/lock_open.png \
  res/icons/overview.png \
  res/icons/qrcode.png \
  res/icons/quit.png \
  res/icons/receive.png \
  res/icons/remove.png \
  res/icons/send.png \
  res/icons/synced.png \
  res/icons/toolbar.png \
  res/icons/toolbar_testnet.png \
  res/icons/transaction0.png \
  res/icons/transaction2.png \
  res/icons/transaction_conflicted.png \
  res/icons/tx_inout.png \
  res/icons/tx_input.png \
  res/icons/tx_output.png \
  res/icons/tx_mined.png

BITMARK_QT_CPP = \
  bitmark.cpp \
  bitmarkaddressvalidator.cpp \
  bitmarkamountfield.cpp \
  bitmarkgui.cpp \
  bitmarkunits.cpp \
  clientmodel.cpp \
  csvmodelwriter.cpp \
  guiutil.cpp \
  intro.cpp \
  monitoreddatamapper.cpp \
  notificator.cpp \
  optionsdialog.cpp \
  optionsmodel.cpp \
  qvalidatedlineedit.cpp \
  qvaluecombobox.cpp \
  rpcconsole.cpp \
  splashscreen.cpp \
  trafficgraphwidget.cpp \
  utilitydialog.cpp \
  winshutdownmonitor.cpp

if ENABLE_WALLET
BITMARK_QT_CPP += \
  addressbookpage.cpp \
  addresstablemodel.cpp \
  askpassphrasedialog.cpp \
  coincontroldialog.cpp \
  coincontroltreewidget.cpp \
  editaddressdialog.cpp \
  openuridialog.cpp \
  overviewpage.cpp \
  paymentrequestplus.cpp \
  paymentserver.cpp \
  receivecoinsdialog.cpp \
  receiverequestdialog.cpp \
  recentrequeststablemodel.cpp \
  sendcoinsdialog.cpp \
  sendcoinsentry.cpp \
  signverifymessagedialog.cpp \
  transactiondesc.cpp \
  transactiondescdialog.cpp \
  transactionfilterproxy.cpp \
  transactionrecord.cpp \
  transactiontablemodel.cpp \
  transactionview.cpp \
  walletframe.cpp \
  walletmodel.cpp \
  walletmodeltransaction.cpp \
  walletview.cpp
endif

RES_IMAGES = \
  res/images/about.png \
  res/images/splash.png \
  res/images/splash_testnet.png

RES_MOVIES = $(wildcard res/movies/spinner-*.png)

BITMARK_RC = res/bitmark-qt-res.rc

libbitmarkqt_a_CPPFLAGS = $(AM_CPPFLAGS) $(QT_INCLUDES) \
  -I$(top_srcdir)/src/qt/forms $(QT_DBUS_INCLUDES)
libbitmarkqt_a_SOURCES = $(BITMARK_QT_CPP) $(BITMARK_QT_H) $(QT_FORMS_UI) \
  $(QT_QRC) $(QT_TS) $(PROTOBUF_PROTO) $(RES_ICONS) $(RES_IMAGES) $(RES_MOVIES)

nodist_libbitmarkqt_a_SOURCES = $(QT_MOC_CPP) $(QT_MOC) $(PROTOBUF_CC) \
  $(PROTOBUF_H) $(QT_QRC_CPP)

BUILT_SOURCES = $(nodist_libbitmarkqt_a_SOURCES)

#Generating these with a half-written protobuf header leads to wacky results.
#This makes sure it's done.
$(QT_MOC): $(PROTOBUF_H)
$(QT_MOC_CPP): $(PROTOBUF_H)

# bitmark-qt binary #
bitmark_qt_CPPFLAGS = $(AM_CPPFLAGS) $(QT_INCLUDES) \
  -I$(top_srcdir)/src/qt/forms
bitmark_qt_SOURCES = bitmark.cpp
if TARGET_DARWIN
  bitmark_qt_SOURCES += $(BITMARK_MM)
endif
if TARGET_WINDOWS
  bitmark_qt_SOURCES += $(BITMARK_RC)
endif
bitmark_qt_LDADD = libbitmarkqt.a $(LIBBITMARK_SERVER)
if ENABLE_WALLET
bitmark_qt_LDADD += $(LIBBITMARK_WALLET)
endif
bitmark_qt_LDADD += $(LIBBITMARK_CLI) $(LIBBITMARK_COMMON) $(LIBLEVELDB) $(LIBMEMENV) \
  $(BOOST_LIBS) $(QT_LIBS) $(QT_DBUS_LIBS) $(QR_LIBS) $(PROTOBUF_LIBS) $(BDB_LIBS)
bitmark_qt_LDFLAGS = $(QT_LDFLAGS)

# forms/foo.h -> forms/ui_foo.h
QT_FORMS_H=$(join $(dir $(QT_FORMS_UI)),$(addprefix ui_, $(notdir $(QT_FORMS_UI:.ui=.h))))

#locale/foo.ts -> locale/foo.qm
QT_QM=$(QT_TS:.ts=.qm)

.PHONY: FORCE
.SECONDARY: $(QT_QM)

bitmarkstrings.cpp: FORCE
	$(MAKE) -C $(top_srcdir)/src qt/bitmarkstrings.cpp

translate: bitmarkstrings.cpp $(QT_FORMS_UI) $(QT_FORMS_UI) $(BITMARK_QT_CPP) $(BITMARK_QT_H) $(BITMARK_MM)
	@test -n $(LUPDATE) || echo "lupdate is required for updating translations"
	@QT_SELECT=$(QT_SELECT) $(LUPDATE) $^ -locations relative -no-obsolete -ts locale/bitmark_en.ts

$(QT_QRC_CPP): $(QT_QRC) $(QT_QM) $(QT_FORMS_H) $(RES_ICONS) $(RES_IMAGES) $(RES_MOVIES) $(PROTOBUF_H)
	@cd $(abs_srcdir); test -f $(RCC) && QT_SELECT=$(QT_SELECT) $(RCC) -name bitmark -o $(abs_builddir)/$@ $< || \
	  echo error: could not build $@
	$(SED) -e '/^\*\*.*Created:/d' $@ > $@.n && mv $@{.n,}
	$(SED) -e '/^\*\*.*by:/d' $@  > $@.n && mv $@{.n,}

CLEANFILES = $(BUILT_SOURCES) $(QT_QM) $(QT_FORMS_H) *.gcda *.gcno
