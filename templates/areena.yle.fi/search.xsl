<?xml version="1.0" encoding="ISO-8859-1"?>

<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

<xsl:template match="/">
<wvmenu>
  <title>Hae Areenasta</title>

  <textfield name="keyword">
    <label>Hakusana</label>
  </textfield>

  <itemlist name="media">
    <label>Media</label>
    <item value="">TV &amp; radio</item>
    <item value="video">TV</item>
    <item value="radio">Radio</item>
  </itemlist>

  <itemlist name="order">
    <label>Järjestys</label>
    <item value="parasosuma">Paras osuma</item>
    <item value="ao">A-Ö</item>
    <item value="uusin">Uusin</item>
    <item value="vielaehdit">Vielä ehdit</item>
  </itemlist>

  <itemlist name="content">
    <label>Tyyppi</label>
    <item value="">Ohjelmat</item>
    <item value="muut">Muut videot ja audiot</item>
  </itemlist>

  <itemlist name="channel">
    <label>Kanava</label>
    <item value="">Kaikki</item>
    <item value="yle-tv1">YLE TV1</item>
    <item value="yle-tv2">YLE TV2</item>
    <item value="yle-teema">YLE Teema</item>
    <item value="yle-fst5">YLE FST5</item>
    <item value="yle-radio-suomi">YLE Radio Suomi</item>
    <item value="ylex">YleX</item>
    <item value="yle-radio-1">YLE Radio 1</item>
    <item value="yle-puhe">YLE Puhe</item>
    <item value="radio-vega">Radio Vega</item>
    <item value="radio-x3m">Radio X3M</item>
    <item value="yle-s-mi-radio">YLE S&#225;mi Radio</item>
    <item value="ylen-klassinen">Ylen klassinen</item>
    <item value="etela-karjalan-radio">Etelä-Karjalan Radio</item>
    <item value="etela-savon-radio">Etelä-Savon Radio</item>
    <item value="kainuun-radio">Kainuun Radio</item>
    <item value="kymenlaakson-radio">Kymenlaakson Radio</item>
    <item value="lahden-radio">Lahden Radio</item>
    <item value="lapin-radio">Lapin Radio</item>
    <item value="oulu-radio">Oulu Radio</item>
    <item value="pohjanmaan-radio">Pohjanmaan Radio</item>
    <item value="pohjois-karjalan-radio">Pohjois-Karjalan Radio</item>
    <item value="radio-hame">Radio Häme</item>
    <item value="radio-ita-uusimaa">Radio Itä-Uusimaa</item>
    <item value="radio-keski-pohjanmaa">Radio Keski-Pohjanmaa</item>
    <item value="radio-keski-suomi">Radio Keski-Suomi</item>
    <item value="radio-perameri">Radio Perämeri</item>
    <item value="radio-savo">Radio Savo</item>
    <item value="satakunnan-radio">Satakunnan Radio</item>
    <item value="tampereen-radio">Tampereen Radio</item>
    <item value="turun-radio">Turun Radio</item>
    <item value="ylen-aikainen">Ylen aikainen</item>
    <item value="ylen-lantinen">Ylen läntinen</item>
    <item value="radio-vega-huvudstadsregionen">Radio Vega Huvudstadsregionen</item>
    <item value="radio-vega-vastnyland">Radio Vega Västnyland</item>
    <item value="radio-vega-aboland">Radio Vega Åboland</item>
    <item value="radio-vega-osterbotten">Radio Vega Österbotten</item>
    <item value="radio-vega-ostnyland">Radio Vega Östnyland</item>
  </itemlist>

  <itemlist name="language">
    <label>Kieli</label>
    <item value="">Kaikki</item>
    <item value="fi">Suomi</item>
    <item value="sv">Ruotsi</item>
    <item value="smi">Saami</item>
    <item value="en">Englanti</item>
  </itemlist>

  <itemlist name="subtitles">
    <label>Tekstitys</label>
    <item value="">Ei rajausta</item>
    <item value="fin">Suomi</item>
    <item value="fih">Suomi (ohjelmatekstitys)</item>
    <item value="swe">Ruotsi</item>
    <item value="swh">Ruotsi (ohjelmatekstitys)</item>
    <item value="smi">Saami</item>
  </itemlist>

  <itemlist name="foreign">
    <label>Esitysoikeudet ulkomailla</label>
    <item value="0">Ei rajausta</item>
    <item value="1">Kyllä</item>
  </itemlist>

  <button>
    <label>Hae</label>
    <submission>wvt:///areena.yle.fi/navigation.xsl?srcurl=http://areena.yle.fi/.json%3Fkpl=24%26q={keyword}%26media={media}%26jarjestys={order}%26sisalto={content}%26kanava={channel}%26kieli={language}%26tekstitys={subtitles}%26ulkomaat={foreign}%26sivu=0&amp;param=seriestitle,Hakutulokset&amp;param=preferseries,false&amp;postprocess=json2xml</submission>
  </button>
</wvmenu>
</xsl:template>

</xsl:stylesheet>
