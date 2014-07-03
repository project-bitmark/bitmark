#include "uritests.h"

#include "guiutil.h"
#include "walletmodel.h"

#include <QUrl>

void URITests::uriTests()
{
    SendCoinsRecipient rv;
    QUrl uri;
    uri.setUrl(QString("bitmark:bGfikGxWjawwxnBQSsinRZXFFQNSoudzk8?req-dontexist="));
    QVERIFY(!GUIUtil::parseBitmarkURI(uri, &rv));

    uri.setUrl(QString("bitmark:bGfikGxWjawwxnBQSsinRZXFFQNSoudzk8?dontexist="));
    QVERIFY(GUIUtil::parseBitmarkURI(uri, &rv));
    QVERIFY(rv.address == QString("bGfikGxWjawwxnBQSsinRZXFFQNSoudzk8"));
    QVERIFY(rv.label == QString());
    QVERIFY(rv.amount == 0);

    uri.setUrl(QString("bitmark:bGfikGxWjawwxnBQSsinRZXFFQNSoudzk8?label=Bitmark Example Address"));
    QVERIFY(GUIUtil::parseBitmarkURI(uri, &rv));
    QVERIFY(rv.address == QString("bGfikGxWjawwxnBQSsinRZXFFQNSoudzk8"));
    QVERIFY(rv.label == QString("Bitmark Example Address"));
    QVERIFY(rv.amount == 0);

    uri.setUrl(QString("bitmark:bGfikGxWjawwxnBQSsinRZXFFQNSoudzk8?amount=0.001"));
    QVERIFY(GUIUtil::parseBitmarkURI(uri, &rv));
    QVERIFY(rv.address == QString("bGfikGxWjawwxnBQSsinRZXFFQNSoudzk8"));
    QVERIFY(rv.label == QString());
    QVERIFY(rv.amount == 100000);

    uri.setUrl(QString("bitmark:bGfikGxWjawwxnBQSsinRZXFFQNSoudzk8?amount=1.001"));
    QVERIFY(GUIUtil::parseBitmarkURI(uri, &rv));
    QVERIFY(rv.address == QString("bGfikGxWjawwxnBQSsinRZXFFQNSoudzk8"));
    QVERIFY(rv.label == QString());
    QVERIFY(rv.amount == 100100000);

    uri.setUrl(QString("bitmark:bGfikGxWjawwxnBQSsinRZXFFQNSoudzk8?amount=100&label=Bitmark Example"));
    QVERIFY(GUIUtil::parseBitmarkURI(uri, &rv));
    QVERIFY(rv.address == QString("bGfikGxWjawwxnBQSsinRZXFFQNSoudzk8"));
    QVERIFY(rv.amount == 10000000000LL);
    QVERIFY(rv.label == QString("Bitmark Example"));

    uri.setUrl(QString("bitmark:bGfikGxWjawwxnBQSsinRZXFFQNSoudzk8?message=Bitmark Example Address"));
    QVERIFY(GUIUtil::parseBitmarkURI(uri, &rv));
    QVERIFY(rv.address == QString("bGfikGxWjawwxnBQSsinRZXFFQNSoudzk8"));
    QVERIFY(rv.label == QString());

    QVERIFY(GUIUtil::parseBitmarkURI("bitmark://bGfikGxWjawwxnBQSsinRZXFFQNSoudzk8?message=Bitmark Example Address", &rv));
    QVERIFY(rv.address == QString("bGfikGxWjawwxnBQSsinRZXFFQNSoudzk8"));
    QVERIFY(rv.label == QString());

    uri.setUrl(QString("bitmark:bGfikGxWjawwxnBQSsinRZXFFQNSoudzk8?req-message=Bitmark Example Address"));
    QVERIFY(GUIUtil::parseBitmarkURI(uri, &rv));

    uri.setUrl(QString("bitmark:bGfikGxWjawwxnBQSsinRZXFFQNSoudzk8?amount=1,000&label=Bitmark Example"));
    QVERIFY(!GUIUtil::parseBitmarkURI(uri, &rv));

    uri.setUrl(QString("bitmark:bGfikGxWjawwxnBQSsinRZXFFQNSoudzk8?amount=1,000.0&label=Bitmark Example"));
    QVERIFY(!GUIUtil::parseBitmarkURI(uri, &rv));
}
