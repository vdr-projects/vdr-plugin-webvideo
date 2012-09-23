<?xml version="1.0" encoding="ISO-8859-1"?>

<xsl:stylesheet version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:str="http://exslt.org/strings">

<xsl:template match="/">
<wvmenu>
  <title>Google video search</title>

  <textfield name="q">
    <label>Search terms</label>
  </textfield>

  <button>
    <label>Search</label>
    <submission>wvt:///video.google.com/searchresults.xsl?srcurl=<xsl:value-of select="str:encode-uri('http://video.google.com/videosearch?q={q}+site:video.google.com&amp;start=0&amp;hl=en', true())"/>&amp;HTTP-header=User-Agent,Mozilla/5.0%20(X11;%20Linux%20x86_64;%20rv:5.0.1)%20Gecko/20100101%20Firefox/5.0.1</submission>
  </button>
</wvmenu>
</xsl:template>

</xsl:stylesheet>
