<?xml version="1.0" encoding="ISO-8859-1"?>

<xsl:stylesheet version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:str="http://exslt.org/strings">

<xsl:template match="/">
<wvmenu>
  <title>Search results</title>

  <xsl:choose>
    <xsl:when test="not(//li[contains(@class, 'g')]//td/a[not(img)])">
      <textarea>
        <label>
          <xsl:text>Your search did not return any results.</xsl:text>
        </label>
      </textarea>
    </xsl:when>

    <xsl:otherwise>
      <xsl:for-each select="//li[@class='g']//td/a[not(img)]">
	<xsl:variable name="url" select="str:decode-uri(substring-after(@href, 'q='))" />

        <xsl:choose>
          <xsl:when test="starts-with($url, 'http://www.youtube.com/')">
	    <link>
	      <label><xsl:value-of select="normalize-space(.)" /></label>
	      <stream>wvt:///www.youtube.com/videopage.xsl?srcurl=<xsl:value-of select="str:encode-uri($url, true())"/></stream>
	      <ref>wvt:///www.youtube.com/description.xsl?srcurl=<xsl:value-of select="str:encode-uri(concat('http://gdata.youtube.com/feeds/api/videos/', substring-after($url, 'v='), '?v=2'), true())"/></ref>
	    </link>
          </xsl:when>

          <xsl:when test="starts-with($url, 'http://video.google.com/')">
	    <link>
	      <label><xsl:value-of select="normalize-space(.)"/></label>
	      <stream>wvt:///video.google.com/videopage.xsl?srcurl=<xsl:value-of select="str:encode-uri($url, true())"/></stream>
	      <ref>wvt:///video.google.com/description.xsl?srcurl=<xsl:value-of select="str:encode-uri($url, true())"/></ref>
	    </link>
          </xsl:when>

          <xsl:when test="starts-with($url, 'http://www.metacafe.com/')">
	    <link>
	      <label><xsl:value-of select="normalize-space(.)"/></label>
	      <stream>wvt:///www.metacafe.com/videopage.xsl?srcurl=<xsl:value-of select="str:encode-uri($url, true())"/></stream>
	      <ref>wvt:///www.metacafe.com/description.xsl?srcurl=<xsl:value-of select="str:encode-uri($url, true())"/></ref>
	    </link>
          </xsl:when>

          <xsl:when test="starts-with($url, 'http://vimeo.com/')">
	    <link>
	      <label><xsl:value-of select="normalize-space(.)"/></label>
	      <stream>wvt:///vimeo.com/video.xsl?srcurl=http://www.vimeo.com/moogaloop/load/clip:<xsl:value-of select="substring-after($url, 'http://vimeo.com/')"/>&amp;HTTP-header=user-agent,Wget/1.2%20%28linux-gnu%29</stream>
	      <ref>wvt:///vimeo.com/description.xsl?srcurl=http://vimeo.com/api/v2/video/<xsl:value-of select="substring-after($url, 'http://vimeo.com/')"/>.xml</ref>
	    </link>
          </xsl:when>

        </xsl:choose>
      </xsl:for-each>

      <xsl:for-each select="id('nav')//td[@class='b']/a">
        <link>
          <label><xsl:value-of select="span[2]/text()"/></label>
          <ref>wvt:///video.google.com/searchresults.xsl?srcurl=<xsl:value-of select="str:encode-uri(@href, true())"/></ref>
        </link>
      </xsl:for-each>
    </xsl:otherwise>
  </xsl:choose>

</wvmenu>
</xsl:template>

</xsl:stylesheet>
