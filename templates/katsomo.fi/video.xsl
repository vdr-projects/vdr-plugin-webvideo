<?xml version="1.0" encoding="UTF-8"?>

<xsl:stylesheet version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:str="http://exslt.org/strings"
  exclude-result-prefixes="str">

<xsl:param name="title">katsomo_video</xsl:param>
<xsl:param name="videopage"/>

<xsl:template name="mediaurl">
  <xsl:param name="title"/>
  <xsl:param name="videopage"/>

  <xsl:variable name="absoluteurl">
    <xsl:choose>
      <xsl:when test="contains($videopage, 'www.katsomo.fi')">
	<xsl:value-of select="$videopage"/>
      </xsl:when>
      <xsl:otherwise>
	<xsl:value-of select="concat('http://www.katsomo.fi', $videopage)"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:variable>

  <mediaurl>
    <title><xsl:value-of select="$title"/></title>
    <url priority="50">
      <xsl:if test="$videopage">wvt:///bin/katsomo-dl?contenttype=video/x-flv&amp;arg=<xsl:value-of select="str:encode-uri($absoluteurl, true())"/></xsl:if>
    </url>
  </mediaurl>
</xsl:template>

<xsl:template match="/">
  <xsl:call-template name="mediaurl">
    <xsl:with-param name="title" select="$title"/>
    <xsl:with-param name="videopage" select="$videopage"/>
  </xsl:call-template>
</xsl:template>

</xsl:stylesheet>
