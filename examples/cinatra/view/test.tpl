<!DOCTYPE html>
<html>
<head>
	<title>{{title}}</title>
</head>
<body>
{% for item in test_loop %}
    {% if length(item) == 3 %}
    <div style="background-color: green;">{{ item }}</div>
    {% else  %}
    <div>{{ item }}</div>
    {% endif %}
{% endfor %}
</body>
</html>