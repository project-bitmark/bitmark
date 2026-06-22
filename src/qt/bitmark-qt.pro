# This file was just made for debugging and generating proper linker flags

CONFIG += qt static

QT += core gui network widgets dbus

TARGET = bitmark-qt

TEMPLATE = app
  
HEADERS += addressbookpage.h \
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

SOURCES += bitmark.cpp \
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
winshutdownmonitor.cpp \
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

FORMS += forms/receiverequestdialog.ui \
forms/aboutdialog.ui \
forms/sendcoinsdialog.ui \
forms/overviewpage.ui \
forms/optionsdialog.ui \
forms/signverifymessagedialog.ui \
forms/askpassphrasedialog.ui \
forms/sendcoinsentry.ui \
forms/coincontroldialog.ui \
forms/rpcconsole.ui \
forms/openuridialog.ui \
forms/transactiondescdialog.ui \
forms/receivecoinsdialog.ui \
forms/helpmessagedialog.ui \
forms/addressbookpage.ui \
forms/editaddressdialog.ui \
forms/intro.ui

TRANSLATIONS = locale/bitmark_mn.ts \
locale/bitmark_fa.ts \
locale/bitmark_sk.ts \
locale/bitmark_eo.ts \
locale/bitmark_kk_KZ.ts \
locale/bitmark_nb.ts \
locale/bitmark_cs.ts \
locale/bitmark_cmn.ts \
locale/bitmark_zh_CN.ts \
locale/bitmark_zh_TW.ts \
locale/bitmark_de.ts \
locale/bitmark_ko_KR.ts \
locale/bitmark_th_TH.ts \
locale/bitmark_pl.ts \
locale/bitmark_es_MX.ts \
locale/bitmark_bs.ts \
locale/bitmark_sv.ts \
locale/bitmark_sr.ts \
locale/bitmark_el_GR.ts \
locale/bitmark_hi_IN.ts \
locale/bitmark_pam.ts \
locale/bitmark_ky.ts \
locale/bitmark_et.ts \
locale/bitmark_vi_VN.ts \
locale/bitmark_zh_HK.ts \
locale/bitmark_be_BY.ts \
locale/bitmark_tr.ts \
locale/bitmark_ja.ts \
locale/bitmark_fi.ts \
locale/bitmark_ca_ES.ts \
locale/bitmark_sah.ts \
locale/bitmark_ro_RO.ts \
locale/bitmark_sq.ts \
locale/bitmark_fa_IR.ts \
locale/bitmark_es_CL.ts \
locale/bitmark_id_ID.ts \
locale/bitmark_da.ts \
locale/bitmark_hr.ts \
locale/bitmark_ca@valencia.ts \
locale/bitmark_vi.ts \
locale/bitmark_lt.ts \
locale/bitmark_hu.ts \
locale/bitmark_sl_SI.ts \
locale/bitmark_uk.ts \
locale/bitmark_ru.ts \
locale/bitmark_fr_CA.ts \
locale/bitmark_es_UY.ts \
locale/bitmark_es.ts \
locale/bitmark_ca.ts \
locale/bitmark_eu_ES.ts \
locale/bitmark_pt_PT.ts \
locale/bitmark_cy.ts \
locale/bitmark_gu_IN.ts \
locale/bitmark_nl.ts \
locale/bitmark_ar.ts \
locale/bitmark_ach.ts \
locale/bitmark_fr.ts \
locale/bitmark_it.ts \
locale/bitmark_la.ts \
locale/bitmark_uz@Cyrl.ts \
locale/bitmark_ms_MY.ts \
locale/bitmark_gl.ts \
locale/bitmark_bg.ts \
locale/bitmark_ka.ts \
locale/bitmark_ur_PK.ts \
locale/bitmark_pt_BR.ts \
locale/bitmark_es_DO.ts \
locale/bitmark_af_ZA.ts \
locale/bitmark_en.ts \
locale/bitmark_lv_LV.ts \
locale/bitmark_he.ts

RESOURCES += bitmark.qrc

INCLUDEPATH += "$$clean_path($$PWD/..)" "$$clean_path($$PWD/../obj)" "$$clean_path($$PWD/../leveldb/include)" "$$clean_path($$PWD/../leveldb/helpers/memenv)"

LIBS += -L"$$clean_path($$PWD/..)" -L"$$clean_path($$PWD/../leveldb)" -L"$$clean_path($$PWD/../secp256k1)" -lbitmark_server -lbitmark_wallet -lbitmark_cli -lbitmark_common -lleveldb -lmemenv -lboost_system-mt -lboost_filesystem-mt -lboost_program_options-mt -lboost_thread_win32-mt -lboost_chrono-mt -ldnsapi -liphlpapi -lssl -lcrypto -lcrypt32 -luxtheme -ldwmapi -ld3d11 -ldxgi -ldxguid -lharfbuzz -lcairo -lgobject-2.0 -lfontconfig -lfreetype -lm -lusp10 -lmsimg32 -lpixman-1 -lffi -lexpat -lbz2 -lpng16 -lharfbuzz_too -lfreetype_too -lglib-2.0 -lshlwapi -lpcre -lintl -liconv -lgdi32 -lcomdlg32 -loleaut32 -limm32 -lmpr -luserenv -lversion -lz -lpcre2-16 -lzstd -lnetapi32 -lws2_32 -ladvapi32 -lkernel32 -lole32 -lshell32 -luuid -luser32 -lwinmm -ldbus-1 -liphlpapi -ldbghelp -lmpr -luserenv -lversion -lz -lpcre2-16 -lzstd -lnetapi32 -lws2_32 -ladvapi32 -lkernel32 -lole32 -lshell32 -luuid -luser32 -lwinmm -lqrencode -lprotobuf -lz -ldb_cxx-4.8 -lsecp256k1 -lsecp256k1_precomputed -lminiupnpc -lminiupnpc -lminiupnpc -lminiupnpc -lcrypt32 -liphlpapi -lshlwapi -lmswsock -lws2_32 -ladvapi32 -lrpcrt4 -luuid -loleaut32 -lole32 -lcomctl32 -lshell32 -lwinmm -lwinspool -lcomdlg32 -lgdi32 -luser32 -lkernel32 -lmingwthrd  -lssl -lcrypto -lz -lws2_32 -lgdi32 -lcrypt32 -lcrypto -lz -lws2_32 -lgdi32 -lcrypt32 -lsodium

DEFINES += HAVE_CONFIG_H DEBUG

QMAKE_CXXFLAGS += -g3 -O0 -pthread

win32: QMAKE_LFLAGS += -mwindows
QMAKE_LFLAGS += -static -static-libgcc -static-libstdc++ -Wl,--stack,8388608 -Wl,--large-address-aware  -Wl,--dynamicbase -Wl,--nxcompat

PROTOS += paymentrequest.proto
include(protobuf.pri)
