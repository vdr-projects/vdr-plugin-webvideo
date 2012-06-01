<?xml version="1.0" encoding="ISO-8859-1"?>

<xsl:stylesheet version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:str="http://exslt.org/strings">

<xsl:include href="common.xsl"/>

<xsl:param name="title"/>

<xsl:template match="/jsondocument/dict">
<xsl:variable name = "typeurl">
  <xsl:call-template name="type_to_path">
    <xsl:with-param name="type" select="series/dict/type"/>
  </xsl:call-template>
</xsl:variable>

<wvmenu>
  <title><xsl:value-of select="normalize-space($title)"/></title>
  <textarea>
    <label><xsl:value-of select="normalize-space(desc)"/></label>
  </textarea>
  <textarea>
    <label>Kesto: <xsl:value-of select="duration"/> min</label>
  </textarea>
  <textarea>
    <label>Julkaistu: <xsl:value-of select="translate(published, 'T', ' ')"/></label>
  </textarea>
  <textarea>
    <label>
      <xsl:choose>
        <xsl:when test="type = 'audio'">Radio</xsl:when>
        <xsl:otherwise>TV</xsl:otherwise>
      </xsl:choose>
    </label>
  </textarea>

  <link>
    <label>Lataa</label>
    <stream>wvt:///areena.yle.fi/video.xsl?param=title,<xsl:value-of select="str:encode-uri($title, true())"/>&amp;param=type,<xsl:value-of select="str:encode-uri(type, true())"/>&amp;param=id,<xsl:value-of select="id"/></stream>
  </link>
</wvmenu>
</xsl:template>

</xsl:stylesheet>
