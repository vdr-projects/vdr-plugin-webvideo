<?xml version="1.0" encoding="utf-8"?>

<xsl:stylesheet version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:str="http://exslt.org/strings"
  exclude-result-prefixes="str">

<xsl:output method="xml" version="1.0" encoding="UTF-8" />

<xsl:template match="a">
  <link>
    <label><xsl:value-of select="."/></label>
    <ref>wvt:///www.ruutu.fi/program.xsl?srcurl=<xsl:value-of select="str:encode-uri(@href, true())"/></ref>
  </link>
</xsl:template>

<xsl:template match="/">
<wvmenu>
  <title>Kaikki sarjat</title>

  <xsl:apply-templates select="//div[@class='has-episodes']/a"/>
</wvmenu>
</xsl:template>

</xsl:stylesheet>
