#!/usr/bin/env bash
set -u

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
CRASH_DIR="${ROOT_DIR}/crash"
TIMESTAMP="$(date +%Y%m%d-%H%M%S)"
LOG_FILE="${CRASH_DIR}/build-failure-${TIMESTAMP}.txt"
TMP_LOG="${CRASH_DIR}/.build-temp-${TIMESTAMP}.log"

mkdir -p "${CRASH_DIR}"

cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Release >"${TMP_LOG}" 2>&1
cmake_status=$?
if [ ${cmake_status} -eq 0 ]; then
  cmake --build "${BUILD_DIR}" -j"$(nproc)" >>"${TMP_LOG}" 2>&1
  build_status=$?
else
  build_status=${cmake_status}
fi

cat "${TMP_LOG}"

if [ ${build_status} -eq 0 ]; then
  rm -f "${TMP_LOG}"
  echo "Build succeeded."
  exit 0
fi

{
  echo "KDEP6Dock build failed at ${TIMESTAMP}."
  echo "Root: ${ROOT_DIR}"
  echo "Build dir: ${BUILD_DIR}"
  echo
  echo "To reproduce manually:"
  echo "  cmake -S . -B build -DCMAKE_BUILD_TYPE=Release"
  echo "  cmake --build build -j\$(nproc)"
  echo
  echo "Captured output:"
  echo "----------------"
  cat "${TMP_LOG}"
} >"${LOG_FILE}"

rm -f "${TMP_LOG}"
echo "Build failed. Details written to ${LOG_FILE}" >&2
exit 1
