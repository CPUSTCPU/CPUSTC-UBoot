#!/bin/sh
set -eu

# NOTE: please read docs/build.md before using this script
# Set this to l1 or l2 before running the script, or override it in the environment.
CPUSTC_CACHE_PROFILE=${CPUSTC_CACHE_PROFILE:-l2}
CPUSTC_PUBLISH_DIR=${CPUSTC_PUBLISH_DIR:-out/published}
CPUSTC_CPU_FREQ_HZ=${CPUSTC_CPU_FREQ_HZ:-50000000}
case "${CPUSTC_CPU_FREQ_HZ}" in
	''|*[!0-9]*)
		echo "CPUSTC_CPU_FREQ_HZ must be a positive integer in Hz" >&2
		exit 2
		;;
esac
if [ "${CPUSTC_CPU_FREQ_HZ}" -eq 0 ]; then
	echo "CPUSTC_CPU_FREQ_HZ must be greater than zero" >&2
	exit 2
fi
export CPUSTC_CPU_FREQ_HZ

case "${CPUSTC_CACHE_PROFILE}" in
	l1|l2)
		CPUSTC_DEFCONFIG="la32rsoc_${CPUSTC_CACHE_PROFILE}_defconfig"
		;;
	*)
		echo "CPUSTC_CACHE_PROFILE must be l1 or l2" >&2
		exit 2
		;;
esac

CPUSTC_BUILD_DIR=${CPUSTC_BUILD_DIR:-out/${CPUSTC_CACHE_PROFILE}}
CPUSTC_GIT_REV=$(git rev-parse --short=12 HEAD)
if test -n "$(git status --porcelain --untracked-files=normal)"; then
	CPUSTC_GIT_REV="${CPUSTC_GIT_REV}-dirty"
fi
CPUSTC_ARTIFACT="u-boot-cpustc-${CPUSTC_CACHE_PROFILE}-${CPUSTC_GIT_REV}-${CPUSTC_CPU_FREQ_HZ}hz.bin"
export ARCH=la32r
export CROSS_COMPILE=/opt/loongarch-toolchain/bin/loongarch32r-linux-gnusf-
if test -f .config || test -d include/config; then
	echo "Removing old in-tree build files before the O= build."
	make mrproper
fi
echo "CPUSTC cache profile: ${CPUSTC_CACHE_PROFILE}"
echo "CPUSTC CPU timer frequency: ${CPUSTC_CPU_FREQ_HZ} Hz"
make O="${CPUSTC_BUILD_DIR}" "${CPUSTC_DEFCONFIG}"
make O="${CPUSTC_BUILD_DIR}" -j8
${CROSS_COMPILE}objdump -lS "${CPUSTC_BUILD_DIR}/u-boot" > "${CPUSTC_BUILD_DIR}/u-boot.S"
${CROSS_COMPILE}objdump -dS "${CPUSTC_BUILD_DIR}/u-boot" > "${CPUSTC_BUILD_DIR}/u-boot.s"
cp "${CPUSTC_BUILD_DIR}/u-boot.bin" "${CPUSTC_BUILD_DIR}/${CPUSTC_ARTIFACT}"
# Keep the published copy easy to address from board tooling.  The build
# directory artifact above retains the full profile/frequency/revision data.
CPUSTC_PUBLISHED_ARTIFACT="u-boot-${CPUSTC_CACHE_PROFILE}-$(date '+%Y%m%d_%H%M%S').bin"
if [ "${#CPUSTC_PUBLISHED_ARTIFACT}" -gt 30 ]; then
	echo "CPUSTC published artifact name exceeds 30 characters: ${CPUSTC_PUBLISHED_ARTIFACT}" >&2
	exit 2
fi
mkdir -p "${CPUSTC_PUBLISH_DIR}"
cp "${CPUSTC_BUILD_DIR}/${CPUSTC_ARTIFACT}" \
	"${CPUSTC_PUBLISH_DIR}/${CPUSTC_PUBLISHED_ARTIFACT}"
echo "CPUSTC artifact: ${CPUSTC_BUILD_DIR}/${CPUSTC_ARTIFACT}"
echo "CPUSTC published artifact: ${CPUSTC_PUBLISH_DIR}/${CPUSTC_PUBLISHED_ARTIFACT}"
