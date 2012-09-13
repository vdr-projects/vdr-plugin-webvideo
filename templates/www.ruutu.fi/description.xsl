<?xml version="1.0" encoding="utf-8"?>

<xsl:stylesheet version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:str="http://exslt.org/strings"
  exclude-result-prefixes="str">

<xsl:param name="docurl"/>

<!-- Convert $seconds to hours:min:sec format -->
<xsl:template name="pretty-print-seconds">
  <xsl:param name="seconds"/>

  <xsl:variable name="sec" select="$seconds mod 60"/>
  <xsl:variable name="min" select="floor($seconds div 60) mod 60"/>
  <xsl:variable name="hour" select="floor($seconds div 3600)"/>

  <xsl:value-of select="concat($hour, ':', format-number($min, '00'), ':', format-number($sec, '00'))"/>
</xsl:template>

<xsl:template match="/">
<wvmenu>
  <title><xsl:value-of select="normalize-space(//h1[@class='node-title'])"/></title>

  <textarea>
    <label>
      <xsl:value-of select="normalize-space(//div[contains(@class, 'field-name-field-season')])"/>
      <xsl:text> </xsl:text>
      <xsl:value-of select="normalize-space(//div[contains(@class, 'field-name-field-episode')])"/>
    </label>
  </textarea>

  <textarea>
    <label><xsl:value-of select="normalize-space(//div[contains(@class, 'field-name-field-starttime')])"/></label>
  </textarea>

  <textarea>
    <label><xsl:value-of select="normalize-space(//div[contains(@class, 'field-name-field-webdescription')])"/></label>
  </textarea>

  <textarea>
    <label>Kesto: <xsl:value-of select="normalize-space(//div[contains(@class, 'field-name-field-duration')])"/>
    </label>
  </textarea>

  <link>
    <label>Lataa</label>
    <stream>wvt:///www.ruutu.fi/videopage.xsl?srcurl=<xsl:value-of select="str:encode-uri($docurl, true())"/></stream>
  </link>
</wvmenu>
</xsl:template>

</xsl:stylesheet>
