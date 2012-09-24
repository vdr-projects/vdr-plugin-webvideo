<?xml version="1.0" encoding="ISO-8859-1"?>

<xsl:stylesheet version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:str="http://exslt.org/strings"
  exclude-result-prefixes="str">

<xsl:template match="/">
<wvmenu>
  <title>MoonTV</title>

  <link>
    <label>MoonTV ohjelmat</label>
    <ref>wvt:///moontv.fi/programlist.xsl?srcurl=http://moontv.fi/ohjelmat&amp;HTTP-header=User-Agent,Mozilla/5.0</ref>
  </link>
  <link>
    <label>MoonTV uusimmat videot</label>
    <ref>wvt:///moontv.fi/recent.xsl?srcurl=http://moontv.fi/ohjelmat&amp;HTTP-header=User-Agent,Mozilla/5.0</ref>
  </link>

</wvmenu>
</xsl:template>

</xsl:stylesheet>
