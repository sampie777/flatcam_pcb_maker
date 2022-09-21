#!/bin/bash

progname=$(basename $0)

function usage {
  cat << HEREDOC

     Usage: $progname [command]

     commands:
       patch                  Release a patch version (0.0.X)
       minor                  Release a minor version (0.X.0)
       setversion <version>   Change version to <version>
       -h, --help             Show this help message and exit

HEREDOC
}

function setVersion() {
    version="$1"
    buildVersion=$(git rev-list HEAD --first-parent --count)

    # todo...
}

function releasePatch {
  git checkout master || exit 1

  # Create patch version
  CURRENT_VERSION=$(git tag --list 'v*' --sort 'v:refname' | tail -n 1)
  RELEASE_VERSION=$(echo ${CURRENT_VERSION} | awk -F'.' '{print $1"."$2"."$3+1}')

  git merge develop || exit 1

  setVersion "${RELEASE_VERSION}" || exit 1

  pushAndRelease "${RELEASE_VERSION}"
}

function releaseMinor {
  git checkout master || exit 1
  git merge develop || exit 1

  # Create patch version
  CURRENT_VERSION=$(git tag --list 'v*' --sort 'v:refname' | tail -n 1)
  RELEASE_VERSION=$(echo ${CURRENT_VERSION} | awk -F'.' '{print $1"."$2".0"}')

  setVersion "${RELEASE_VERSION}" || exit 1

  pushAndRelease "${RELEASE_VERSION}"
}

function pushAndRelease {
  RELEASE_VERSION="$1"
  echo "Release version: ${RELEASE_VERSION}"

  git commit -m "version release: ${RELEASE_VERSION}"
  git tag "v${RELEASE_VERSION}" || exit 1
  git push -u origin master --tags || exit 1
}

function setNextDevelopmentVersion {
  git checkout develop || exit 1
  git rebase master || exit 1

  # Generate next (minor) development version
  CURRENT_VERSION=$(git tag --list 'v*' --sort 'v:refname' | tail -n 1)
  DEV_VERSION=$(echo ${CURRENT_VERSION} | awk -F'.' '{print $1"."$2+1".0-SNAPSHOT"}')

  echo "Next development version: ${DEV_VERSION}"
  setVersion "${DEV_VERSION}" || exit 1

  git commit -m "next development version" || exit 1
  git push -u origin develop --tags || exit 1
}

command="$1"
case $command in
  patch)
    releasePatch
    setNextDevelopmentVersion
    ;;
  minor)
    releaseMinor
    setNextDevelopmentVersion
    ;;
  setversion)
    setVersion "$2"
    ;;
  -h|--help)
    usage
    ;;
  *)
    echo "Invalid command"
    exit 1
    ;;
esac
