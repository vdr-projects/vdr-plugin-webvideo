<?xml version="1.0" encoding="ISO-8859-1"?>

<xsl:stylesheet version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:str="http://exslt.org/strings"
  xmlns:map="mapping"
  exclude-result-prefixes="str map">

<!-- Mapping from Youtube fmt parameter (key) to webvideo priority
     score (value) -->
<map:associativearray>
  <map:item key="17" value="40"/> <!-- 176x144 3GP   -->
  <map:item key="5" value="50"/>  <!-- 320x240 FLV   -->
  <map:item key="34" value="52"/> <!-- 640x360 FLV   -->
  <map:item key="35" value="54"/> <!-- 854x480 FLV   -->
  <map:item key="18" value="60"/> <!-- 640x360 MP4   -->
  <map:item key="43" value="65"/> <!-- 640x360 WebM  -->
  <map:item key="44" value="67"/> <!-- 854x480 WebM  -->
  <map:item key="22" value="70"/> <!-- 720p MP4      -->
  <map:item key="45" value="75"/> <!-- 720p WebM     -->
  <map:item key="37" value="80"/> <!-- 1080p MP4     -->
</map:associativearray>

<xsl:template name="fmturl">
  <xsl:variable name="fmt">
    <xsl:for-each select="str:tokenize(., '&amp;')">
      <xsl:if test="starts-with(., 'itag=')">
	<xsl:value-of select="substring-after(., 'itag=')"/>
      </xsl:if>
    </xsl:for-each>
  </xsl:variable>
  <xsl:variable name="url">
    <xsl:for-each select="str:tokenize(., '&amp;')">
      <xsl:if test="starts-with(., 'url=')">
	<xsl:value-of select="str:decode-uri(substring-after(., 'url='))"/>
      </xsl:if>
    </xsl:for-each>
  </xsl:variable>
  <xsl:variable name="signature">
    <xsl:for-each select="str:tokenize(., '&amp;')">
      <xsl:if test="starts-with(., 'sig=')">
	<xsl:value-of select="str:decode-uri(substring-after(., 'sig='))"/>
      </xsl:if>
    </xsl:for-each>
  </xsl:variable>

  <xsl:if test="$url">
    <url>
      <xsl:attribute name="priority">
	<xsl:choose>
	  <xsl:when test="document('')/*/map:associativearray/map:item[@key=$fmt]">
	    <xsl:value-of select="document('')/*/map:associativearray/map:item[@key=$fmt]/@value"/>
	  </xsl:when>
	  <xsl:otherwise>
	    <xsl:value-of select="50"/>
	  </xsl:otherwise>
	</xsl:choose>
      </xsl:attribute>
      <xsl:value-of select="concat($url, '&amp;signature=', $signature)"/>
    </url>
  </xsl:if>
</xsl:template>

<xsl:template name="mediaurl">
  <xsl:param name="videoinfo"/>

  <mediaurl>
    <title>
      <xsl:variable name="titleparam">
	<xsl:choose>
	  <xsl:when test="contains(substring-after($videoinfo, '&amp;title='), '&amp;')">
	    <xsl:value-of select="substring-before(substring-after($videoinfo, '&amp;title='), '&amp;')"/>
	  </xsl:when>
	  <xsl:otherwise>
	    <xsl:value-of select="substring-after($videoinfo, '&amp;title=')"/>
	  </xsl:otherwise>
	</xsl:choose>
      </xsl:variable>
      
      <xsl:value-of select="str:decode-uri(str:replace($titleparam, '+', ' '))"/>
    </title>

    <xsl:for-each select="str:split(str:decode-uri(substring-before(substring-after($videoinfo, '&amp;url_encoded_fmt_stream_map='), '&amp;')), ',')">
      <xsl:call-template name="fmturl"/>
    </xsl:for-each>

  </mediaurl>
</xsl:template>

<xsl:template name="main">
  <xsl:call-template name="mediaurl">
    <xsl:with-param name="videoinfo" select="."/>
  </xsl:call-template>
</xsl:template>

<xsl:template match="/">
  <xsl:call-template name="main"/>
</xsl:template>

</xsl:stylesheet>
