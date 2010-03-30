/* And all that Malarkey // Trimming form elements

Please feel free to use this JavaScript file in any way that you like, although please
keep the comments intact and a link back to http://www.stuffandnonsense.co.uk/archives/trimming_form_fields.html
would be appreciated.

If you come up with a stunning design based on this technique, it would be really nice
if you would post a comment containing a URL on 
http://www.stuffandnonsense.co.uk/archives/trimming_form_fields.html#Comments 

Thanks to Brothercake (http://www.brothercake.com/) for his help with the Javascript.
His fantastic UDM 4 fully-featured and accessible website menu system is a must!
(http://www.udm4.com/) */

window.onload = function()
{
	if(document.getElementById)
	{
		var linkContainer = document.getElementById('fm-intro');
		var linebreak = linkContainer.appendChild(document.createElement('br'));
		var toggle = linkContainer.appendChild(document.createElement('a'));
		toggle.href = '#';
		toggle.appendChild(document.createTextNode(' Hide optional fields?'));
		toggle.onclick = function()
		{
			var linkText = this.firstChild.nodeValue;
			this.firstChild.nodeValue = (linkText == ' Hide optional fields?') ? ' Display optional fields?' : ' Hide optional fields?';
			
			var tmp = document.getElementsByTagName('div');
			for (var i=0;i<tmp.length;i++)
			{
				if(tmp[i].className == 'fm-opt')
				{
					tmp[i].style.display = (tmp[i].style.display == 'none') ? 'block' : 'none';
				}
			}
			return false;
		}
	}
}