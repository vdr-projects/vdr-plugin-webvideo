<?xml version="1.0" encoding="ISO-8859-1"?>

<xsl:stylesheet version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:str="http://exslt.org/strings"
  exclude-result-prefixes="str">

<xsl:template match="/">
<wvmenu>
  <title>MoonTV uusimmat videot</title>

  <xsl:for-each select="//div[@class='thumbnails']/div">
    <link>
      <label><xsl:value-of select="h6"/></label>
      <ref>wvt:///moontv.fi/description.xsl?srcurl=<xsl:value-of select="str:encode-uri(a/@href, true())"/>&amp;HTTP-header=User-Agent,Mozilla/5.0</ref>
      <stream>wvt:///moontv.fi/videopage.xsl?srcurl=<xsl:value-of select="str:encode-uri(a/@href, true())"/>&amp;HTTP-header=User-Agent,Mozilla/5.0</stream>
    </link>
  </xsl:for-each>
</wvmenu>
</xsl:template>

</xsl:stylesheet>
