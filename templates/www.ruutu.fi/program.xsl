<?xml version="1.0" encoding="utf-8"?>

<xsl:stylesheet version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:str="http://exslt.org/strings"
  exclude-result-prefixes="str">

<xsl:param name="docurl"/>

<xsl:template match="article">
  <link>
    <label>
      <xsl:value-of select="normalize-space(.//h2[@class='node-title']/a)"/>
      <xsl:if test=".//div[contains(@class, 'field-name-field-xb-name')]">
        <xsl:text> </xsl:text>
        <xsl:value-of select="normalize-space(.//div[contains(@class, 'field-name-field-xb-name')])"/>
      </xsl:if>
      <xsl:text> </xsl:text>
      <xsl:value-of select="normalize-space(.//div[contains(@class, 'field-name-field-season')])"/>
      <xsl:text> </xsl:text>
      <xsl:value-of select="normalize-space(.//div[contains(@class, 'field-name-field-episode')])"/>
    </label>
    <ref>wvt:///www.ruutu.fi/description.xsl?srcurl=<xsl:value-of select=".//h2[@class='node-title']/a/@href"/></ref>
    <stream>wvt:///www.ruutu.fi/videopage.xsl?srcurl=<xsl:value-of select=".//h2[@class='node-title']/a/@href"/></stream>
  </link>
</xsl:template>

<xsl:template match="/">
<wvmenu>
  <!-- title -->
  <title><xsl:value-of select="//h1[@class='node-title']"/></title>

  <!-- Video links -->
  <xsl:if test="not(id('block-views-videos-by-series-episodes-1')//article)">
    <textarea>
      <label>Ei jaksoja</label>
    </textarea>
  </xsl:if>

  <xsl:apply-templates select="id('block-views-videos-by-series-episodes-1')//article">
  </xsl:apply-templates>

  <xsl:if test="id('block-views-videos-by-series-clips-1')//article">
    <textarea>
      <label>Klipit</label>
    </textarea>
    <xsl:apply-templates select="id('block-views-videos-by-series-clips-1')//article">
    </xsl:apply-templates>
  </xsl:if>
</wvmenu>
</xsl:template>

</xsl:stylesheet>
