<?xml version="1.0" encoding="utf-8"?>

<xsl:stylesheet version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:str="http://exslt.org/strings"
  exclude-result-prefixes="str">

<xsl:import href="program.xsl"/>

<xsl:template match="/">
<wvmenu>
  <title>Ruutu.fi - Katsotuimmat</title>

  <xsl:apply-templates select="id('quicktabs-container-katsotuimmat')//article"/>
</wvmenu>
</xsl:template>

</xsl:stylesheet>
