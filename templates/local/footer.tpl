<tax-invoice/>
{if $host_name == "smithlecture.org"}
  {include file="local/smith_footer.tpl" }
{elseif $host_name == "citywoman.org.au"}
  {include file="local/citywoman_footer.tpl" }
{else}
  {include file="local/citybibleforum_footer.tpl" }
{/if}
