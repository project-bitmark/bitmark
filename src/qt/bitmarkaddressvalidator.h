// Copyright (c) 2011-2014 The Bitcoin Core Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITMARKADDRESSVALIDATOR_H
#define BITMARKADDRESSVALIDATOR_H

#include <QValidator>

/** Base58 entry widget validator, checks for valid characters and
 * removes some whitespace.
 */
class BitmarkAddressEntryValidator : public QValidator
{
    Q_OBJECT

public:
    explicit BitmarkAddressEntryValidator(QObject *parent);

    State validate(QString &input, int &pos) const;
};

/** Bitmark address widget validator, checks for a valid bitmark address.
 */
class BitmarkAddressCheckValidator : public QValidator
{
    Q_OBJECT

public:
    explicit BitmarkAddressCheckValidator(QObject *parent);

    State validate(QString &input, int &pos) const;
};

#endif // BITMARKADDRESSVALIDATOR_H
