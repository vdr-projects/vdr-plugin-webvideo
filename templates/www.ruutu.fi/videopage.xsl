<?xml version="1.0" encoding="UTF-8"?>

<xsl:stylesheet version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:str="http://exslt.org/strings"
  exclude-result-prefixes="str">

<xsl:import href="video.xsl"/>

<xsl:param name="docurl"/>

<xsl:template match="/" mode="video_config">
  <xsl:call-template name="mediaurl"/>
</xsl:template>

<xsl:template match="/">
  <xsl:variable name="videourl"><xsl:value-of select="translate(str:tokenize(substring-before(substring-after(id('block-ruutu-commerce-video-area')/script[contains(., 'ruutuplayer(')], '('), ')'), ',')[5], ' &quot;', '')"/></xsl:variable>

  <xsl:apply-templates select="document($videourl)" mode="video_config"/>
</xsl:template>

</xsl:stylesheet>
