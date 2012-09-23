<?xml version="1.0" encoding="ISO-8859-1"?>

<xsl:stylesheet version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:str="http://exslt.org/strings">

<xsl:template match="a">
  <link>
    <label><xsl:value-of select="."/></label>
    <stream>wvt:///video.google.com/videopage.xsl?srcurl=<xsl:value-of select="str:encode-uri(@href, true())"/></stream>
  </link>
</xsl:template>

<xsl:template match="/">
<wvmenu>
  <title>Search results</title>

  <xsl:choose>
    <xsl:when test="not(//a[starts-with(@href, 'http://video.google.com/videoplay?')])">
      <textarea>
        <label>
          <xsl:text>Your search did not return any results.</xsl:text>
        </label>
      </textarea>
    </xsl:when>

    <xsl:otherwise>
      <xsl:apply-templates select="//a[starts-with(@href, 'http://video.google.com/videoplay?')]"/>
    </xsl:otherwise>
  </xsl:choose>

</wvmenu>
</xsl:template>

</xsl:stylesheet>
