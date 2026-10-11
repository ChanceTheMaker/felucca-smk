// SPDX-License-Identifier: GPL-3.0-only
// Immutable research mirror of the official vendor files. Package validation
// checks the complete SHA-256 before any firmware package can be prepared.
const base='https://raw.githubusercontent.com/jonathaslacerda/smk-37-pro-docs/ab85d7340163ba9e171dc9396e31c034829bbc02/firmware/';
export const STOCK_SOURCES=Object.freeze({
  'SMK-37 Pro':base+'smk37pro/SMK-37_Pro_016.fwsc',
  'SMK-37 Elite':base+'smk37elite/SMK-37_Elite_016.fwsc'
});
