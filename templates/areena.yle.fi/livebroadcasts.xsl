<?xml version="1.0" encoding="ISO-8859-1"?>

<xsl:stylesheet version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:str="http://exslt.org/strings">

<xsl:template match="/jsondocument/dict/current/list/li/dict/pubContent/dict">
  <link>
    <label><xsl:value-of select="title"/></label>
    <ref>wvt:///areena.yle.fi/description.xsl?srcurl=<xsl:value-of select="str:encode-uri(concat('http://areena.yle.fi/tv/', id, '.json'), true())"/>&amp;param=title,<xsl:value-of select="str:encode-uri(title, true())"/>&amp;postprocess=json2xml</ref>
    <stream>wvt:///areena.yle.fi/video.xsl?srcurl=&amp;param=title,<xsl:value-of select="str:encode-uri(title, true())"/>&amp;param=type,<xsl:value-of select="type"/>&amp;param=id,<xsl:value-of select="id"/></stream>
  </link>
</xsl:template>

<xsl:template match="/jsondocument/dict/upcoming/list/li/dict/items/list/li/dict">
  <textarea>
    <label><xsl:value-of select="concat(translate(start, 'T', ' '), ' ', pubContent/dict/title)"/></label>
  </textarea>
</xsl:template>

<xsl:template match="/">
<wvmenu>
  <title>Suorat lähetykset</title>

  <textarea>
    <label>Nyt suorana</label>
  </textarea>
  <xsl:apply-templates select="/jsondocument/dict/current/list/li/dict/pubContent/dict"/>

  <textarea>
    <label>Tulossa</label>
  </textarea>
  <xsl:apply-templates select="/jsondocument/dict/upcoming/list/li/dict/items/list/li/dict"/>

</wvmenu>
</xsl:template>

</xsl:stylesheet>
