#pragma once

/**
 * @brief The traverseDocumentWithSvgpp function traverses the SVG
 * document through the SVG++ document loader and provides it with a
 * context containing callbacks for handling SVG elements. It is
 * separated from the SvgppContext class to reduce the compile time of
 * the SvgppContext class.
 */

class SvgppContext;

void traverseDocumentWithSvgpp(SvgppContext &context, char *text);
