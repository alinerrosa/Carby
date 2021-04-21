echo "Envio de dados para Google Forms"
while true;
do
  cat database.csv | while IFS=';' read a b;
  do
    echo "$a:$b"
    formid="1_q2E8SroEc5OOQh54l85XWK4mJrh9U7zLE_mN11dT_I"
    curl https://docs.google.com/forms/d/$formid/formResponse -d ifq -d "entry.1076682711= $a" -d "entry.1505376468= $b" -d "entry.1031950862= 430" -d submit=Submit;
    sleep 10
  done < <(tail -n +2 database.csv)
done
