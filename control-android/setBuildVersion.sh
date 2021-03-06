#!/bin/bash

#CODE=`git tag | grep -c ^v[0-9]`
#NAME=`git describe --dirty | sed -e 's/^v//'`
#COMMITS=`echo ${NAME} | sed -e 's/[0-9\.]*//'`
#if [ "x${COMMITS}x" = "xx" ] ; then
#	VERSION="${NAME}"
#else
#	BRANCH=" (`git branch | grep "^\*" | sed -e 's/^..//'`)"
#	VERSION="${NAME}${BRANCH}"
#fi
#
CODE=$(git rev-list master --first-parent --count)
VERSION="dev"

PROTOCOL_VERSION=$(grep '^version' ./src/protocol/protocol.dat)
if [ $? != 0 ] ; then
	echo "error: protocol.dat version not found [1]">&2
	exit 1
fi
REGEX="^version=([0-9.]*):I"
if [[ "$PROTOCOL_VERSION" =~ $REGEX ]] ; then
	# echo "match: ${BASH_REMATCH[1]}"
	VERSION="${BASH_REMATCH[1]}-$CODE-$VERSION"
else
	echo "error: protocol.dat regex error">&2
	exit 1
fi

echo "   Code: ${CODE}"
echo "   Ver:  ${VERSION}"

cat AndroidManifest.xml | \
	sed -e "s/android:versionCode=\"[0-9][0-9]*\"/android:versionCode=\"${CODE}\"/" \
		-e "s/android:versionName=\".*\"/android:versionName=\"${VERSION}\"/" \
	> bin/AndroidManifest.xml
