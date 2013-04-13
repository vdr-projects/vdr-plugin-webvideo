<?xml version="1.0" encoding="UTF-8"?>

<xsl:stylesheet version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:str="http://exslt.org/strings"
  exclude-result-prefixes="str">

<!-- Create video link -->
<xsl:template name="video_link">
  <xsl:param name="title"/>
  <xsl:param name="href"/>

  <link>
    <label><xsl:value-of select="$title"/></label>
    <ref>wvt:///katsomo.fi/description.xsl?srcurl=<xsl:value-of select="str:encode-uri($href, true())"/></ref>
    <stream>wvt:///katsomo.fi/video.xsl?param=videopage,<xsl:value-of select="str:encode-uri($href, true())"/>&amp;param=title,<xsl:value-of select="str:encode-uri($title, true())"/></stream>
  </link>
</xsl:template>

<xsl:template match="id('search_episodes')/div">
  <xsl:call-template name="video_link">
    <xsl:with-param name="title" select="concat(normalize-space(h4/a), ' ', p[@class='date'])"/>
    <xsl:with-param name="href" select="h4/a/@href"/>
  </xsl:call-template>
</xsl:template>

<xsl:template match="id('search_clips')/div">
  <xsl:call-template name="video_link">
    <xsl:with-param name="title" select="normalize-space(concat(p/a[1], ' ', h4/a))"/>
    <xsl:with-param name="href" select="h4/a/@href"/>
  </xsl:call-template>
</xsl:template>

<xsl:template match="/">
<wvmenu>
  <title><xsl:value-of select="/html/head/title"/></title>

  <!-- Program search results require javascript. This is not a -->
  <!-- problem because all programs are listed on the main page -->
  <!-- anyway. -->
  <!-- <xsl:apply-templates select="id('search_results')/div[contains(@class, 'item')]"/> -->

  <textarea>
    <label>Jaksot</label>
  </textarea>
  <xsl:apply-templates select="id('search_episodes')/div[contains(@class, 'item')]"/>

  <textarea>
    <label>Klipit</label>
  </textarea>
  <xsl:apply-templates select="id('search_clips')/div[contains(@class, 'item')]"/>
</wvmenu>
</xsl:template>

</xsl:stylesheet>
